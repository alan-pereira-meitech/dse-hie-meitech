import asyncio
import json
import logging
import os
from collections import defaultdict
from typing import Any, Dict, List, Optional, Set

from fastapi import FastAPI, HTTPException, WebSocket, WebSocketDisconnect
from pydantic import BaseModel

SCALE_COMMAND_TARE = 1701994868
SCALE_COMMAND_ZERO = 1869768058
SCALE_COMMAND_GROSS = 1936683623
SCALE_STATUS_ONGOING = 1634168417
SCALE_STATUS_OK = 1801543519

app = FastAPI(title="DSE Jet Simulation Server", version="0.1.0")


class JetState:
    def __init__(self) -> None:
        self._values: Dict[str, Any] = {
            "6002/01": 0,
            "6002/02": SCALE_STATUS_OK,
            "6012/01": 0,
            "601A/01": 0,
            "6144/00": 0,
            "6040/02": 0,
            "6040/03": 0,
            "6040/04": 0,
            "6040/05": 0,
            "3321/00": 0,
            "3322/00": 0,
            "3323/00": 0,
            "3324/00": 0,
            "3331/00": 0,
            "3332/00": 0,
            "3333/00": 0,
            "3334/00": 0,
        }
        self._subscriptions: Dict[str, Set[WebSocket]] = defaultdict(set)
        self._connections: Dict[WebSocket, Set[str]] = defaultdict(set)
        self._lock = asyncio.Lock()
        self.logger = logging.getLogger("JetState")

    async def register(self, websocket: WebSocket) -> None:
        async with self._lock:
            self._connections.setdefault(websocket, set())

    async def unregister(self, websocket: WebSocket) -> None:
        async with self._lock:
            paths = self._connections.pop(websocket, set())
            for path in paths:
                sockets = self._subscriptions.get(path)
                if sockets is None:
                    continue
                sockets.discard(websocket)
                if not sockets:
                    self._subscriptions.pop(path, None)

    async def add_subscription(self, websocket: WebSocket, path: str) -> None:
        async with self._lock:
            self._connections[websocket].add(path)
            self._subscriptions[path].add(websocket)

    async def remove_subscription(self, websocket: WebSocket, path: str) -> None:
        async with self._lock:
            if websocket in self._connections:
                self._connections[websocket].discard(path)
            sockets = self._subscriptions.get(path)
            if sockets is None:
                return
            sockets.discard(websocket)
            if not sockets:
                self._subscriptions.pop(path, None)

    async def get_value(self, path: str) -> Optional[Any]:
        async with self._lock:
            return self._values.get(path)

    async def set_value(self, path: str, value: Any, *, broadcast: bool = True) -> None:
        async with self._lock:
            self._values[path] = value
            subscribers = list(self._subscriptions.get(path, set()))
        if broadcast:
            await self._broadcast(subscribers, path, value)

    async def snapshot(self) -> Dict[str, Any]:
        async with self._lock:
            return dict(self._values)

    async def _broadcast(self, subscribers: List[WebSocket], path: str, value: Any) -> None:
        payload = json.dumps({"type": "event", "path": path, "event": "change", "value": value})
        for websocket in subscribers:
            try:
                await websocket.send_text(payload)
            except Exception as exc:  # pragma: no cover - best effort cleanup
                self.logger.warning("Failed to notify subscriber %s: %s", websocket.client, exc)
                await self.unregister(websocket)


state = JetState()


class ValuePayload(BaseModel):
    value: Any


@app.get("/jet/state")
async def get_state() -> Dict[str, Any]:
    return await state.snapshot()


@app.post("/jet/state/{path:path}")
async def set_state(path: str, payload: ValuePayload):
    await state.set_value(path, payload.value)
    return {"path": path, "value": payload.value}


@app.post("/jet/simulate/weight")
async def simulate_weight(payload: ValuePayload):
    value = payload.value
    if not isinstance(value, dict):
        raise HTTPException(status_code=400, detail="value must be an object with gross/net/status fields")
    gross = value.get("gross", 0)
    net = value.get("net", gross)
    status = value.get("status", 0)
    await state.set_value("6144/00", gross)
    await state.set_value("601A/01", net)
    await state.set_value("6012/01", status)
    return {"gross": gross, "net": net, "status": status}


async def _complete_scale_command(command: int) -> None:
    await state.set_value("6002/02", SCALE_STATUS_ONGOING)
    await asyncio.sleep(0.2)
    if command == SCALE_COMMAND_TARE:
        await state.set_value("601A/01", 0)
    elif command == SCALE_COMMAND_ZERO:
        await state.set_value("6144/00", 0)
    elif command == SCALE_COMMAND_GROSS:
        # Example: set gross to previously stored net
        net = await state.get_value("601A/01") or 0
        await state.set_value("6144/00", net)
    await asyncio.sleep(0.2)
    await state.set_value("6002/02", SCALE_STATUS_OK)


async def handle_authenticate(message: Dict[str, Any]) -> Dict[str, Any]:
    params = message.get("params", {})
    user = params.get("user", "")
    if not user:
        return {
            "type": "error",
            "id": message.get("id"),
            "error": {"message": "missing user"},
        }
    return {
        "type": "result",
        "id": message.get("id"),
        "success": True,
        "value": {"session": "demo"},
    }


async def handle_fetch(message: Dict[str, Any], websocket: WebSocket) -> Dict[str, Any]:
    path = message.get("path")
    if not path:
        return {
            "type": "error",
            "id": message.get("id"),
            "error": {"message": "missing path"},
        }
    value = await state.get_value(path)
    if value is None:
        return {
            "type": "error",
            "id": message.get("id"),
            "error": {"message": f"unknown path {path}"},
        }
    params = message.get("params") or {}
    subscribe = bool(params.get("subscribe"))
    if subscribe:
        await state.add_subscription(websocket, path)
    return {
        "type": "result",
        "id": message.get("id"),
        "success": True,
        "value": value,
    }


async def handle_set(message: Dict[str, Any]) -> Dict[str, Any]:
    path = message.get("path")
    if not path:
        return {
            "type": "error",
            "id": message.get("id"),
            "error": {"message": "missing path"},
        }
    if "value" not in message:
        return {
            "type": "error",
            "id": message.get("id"),
            "error": {"message": "missing value"},
        }
    value = message.get("value")
    await state.set_value(path, value)
    if path == "6002/01" and isinstance(value, int):
        asyncio.create_task(_complete_scale_command(value))
    return {
        "type": "result",
        "id": message.get("id"),
        "success": True,
        "value": value,
    }


CALL_HANDLERS = {
    "authenticate": handle_authenticate,
    "fetch": handle_fetch,
    "set": handle_set,
}


@app.websocket("/jet/canopen")
async def jet_websocket(websocket: WebSocket):
    await websocket.accept()
    await state.register(websocket)
    try:
        while True:
            data = await websocket.receive_text()
            try:
                message = json.loads(data)
            except json.JSONDecodeError:
                await websocket.send_text(json.dumps({
                    "type": "error",
                    "error": {"message": "invalid json"},
                }))
                continue
            if message.get("type") != "call":
                await websocket.send_text(json.dumps({
                    "type": "error",
                    "id": message.get("id"),
                    "error": {"message": "unsupported frame"},
                }))
                continue
            method = message.get("method")
            handler = CALL_HANDLERS.get(method)
            if handler is None:
                await websocket.send_text(json.dumps({
                    "type": "error",
                    "id": message.get("id"),
                    "error": {"message": f"unknown method {method}"},
                }))
                continue
            if handler is handle_fetch:
                response = await handler(message, websocket)
            else:
                response = await handler(message)
            await websocket.send_text(json.dumps(response))
    except WebSocketDisconnect:
        pass
    finally:
        await state.unregister(websocket)


if __name__ == "__main__":
    import uvicorn

    host = os.getenv("JET_SIM_HOST", "0.0.0.0")
    port = int(os.getenv("JET_SIM_PORT", "8000"))
    uvicorn.run("main:app", host=host, port=port, reload=False)
