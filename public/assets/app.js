(() => {
  const connectionBadge = document.getElementById("connection-status");
  const connectBtn = document.getElementById("connect-btn");
  const disconnectBtn = document.getElementById("disconnect-btn");
  const lastUpdateEl = document.getElementById("last-update");
  const logPanel = document.getElementById("log-panel");
  const actionsContainer = document.getElementById("actions-container");
  const statusStrip = document.getElementById("status-strip");
  const netValueEl = document.getElementById("net-value");
  const netPrintableEl = document.getElementById("net-printable");
  const grossValueEl = document.getElementById("gross-value");
  const grossPrintableEl = document.getElementById("gross-printable");
  const tareValueEl = document.getElementById("tare-value");
  const tarePrintableEl = document.getElementById("tare-printable");
  const unitValueEl = document.getElementById("unit-value");
  const decimalsValueEl = document.getElementById("decimals-value");

  let currentState = { connected: false, snapshot: null };
  let actions = [];
  const actionForms = [];

  function updateConnectionState(state) {
    currentState = state;
    const connected = Boolean(state?.connected);
    if (connectionBadge) {
      connectionBadge.textContent = connected ? "Conectado" : "Desconectado";
      connectionBadge.style.background = connected ? "rgba(74, 222, 128, 0.18)" : "rgba(248, 113, 113, 0.18)";
      connectionBadge.style.color = connected ? "#4ade80" : "#f87171";
    }
    if (connectBtn) connectBtn.disabled = connected;
    if (disconnectBtn) disconnectBtn.disabled = !connected;
    setActionsDisabled(!connected);
    if (!connected && lastUpdateEl) {
      lastUpdateEl.textContent = "—";
    }
  }

  function updateProcessData(snapshot) {
    if (!snapshot) return;
    if (lastUpdateEl) {
      lastUpdateEl.textContent = `Atualizado: ${new Date(snapshot.timestamp).toLocaleTimeString()}`;
    }
    if (netValueEl) netValueEl.textContent = snapshot?.printableWeight?.net ?? "--";
    if (netPrintableEl)
      netPrintableEl.textContent = snapshot?.unit ? `${snapshot.printableWeight.net} ${snapshot.unit}` : snapshot?.printableWeight?.net ?? "--";
    if (grossValueEl) grossValueEl.textContent = snapshot?.printableWeight?.gross ?? "--";
    if (grossPrintableEl)
      grossPrintableEl.textContent = snapshot?.unit ? `${snapshot.printableWeight.gross} ${snapshot.unit}` : snapshot?.printableWeight?.gross ?? "--";
    if (tareValueEl) tareValueEl.textContent = snapshot?.printableWeight?.tare ?? "--";
    if (tarePrintableEl)
      tarePrintableEl.textContent = snapshot?.unit ? `${snapshot.printableWeight.tare} ${snapshot.unit}` : snapshot?.printableWeight?.tare ?? "--";
    if (unitValueEl) unitValueEl.textContent = snapshot?.unit ?? "--";
    if (decimalsValueEl) decimalsValueEl.textContent = `Decimais: ${snapshot?.decimals ?? 0}`;

    if (statusStrip) {
      statusStrip.innerHTML = "";
      const statusEntries = [
        { label: `Modo de tara: ${snapshot?.tareMode ?? "Nenhum"}`, active: snapshot?.tareMode && snapshot.tareMode !== "Nenhum" && snapshot.tareMode !== "None" },
        { label: "Peso estável", active: Boolean(snapshot?.status?.weightStable) },
        { label: "Requer zero", active: Boolean(snapshot?.status?.zeroRequired) },
        { label: "Centro de zero", active: Boolean(snapshot?.status?.centerOfZero) },
        { label: "Dentro da zona zero", active: Boolean(snapshot?.status?.insideZero) },
        { label: "Pronto para uso comercial", active: Boolean(snapshot?.status?.legalForTrade) },
        { label: "Sobrecarga", active: Boolean(snapshot?.status?.overload) },
        { label: "Subcarga", active: Boolean(snapshot?.status?.underload) },
        { label: "Limite seguro elevado", active: Boolean(snapshot?.status?.higherSafeLoadLimit) },
        { label: "Erro geral", active: Boolean(snapshot?.status?.generalScaleError) },
        { label: "Alarme de balança", active: Boolean(snapshot?.status?.scaleAlarm) }
      ];
      statusEntries.forEach((entry) => {
        const pill = document.createElement("span");
        pill.className = "status-pill" + (entry.active ? " active" : "");
        pill.textContent = entry.label;
        statusStrip.appendChild(pill);
      });
    }
  }

  function logActionResult(kind, title, message) {
    if (!logPanel) {
      const logger = kind === "error" ? console.error : kind === "success" ? console.log : console.info;
      logger(`[${title}] ${message}`);
      return;
    }
    const entry = document.createElement("p");
    entry.className = `log-entry log-${kind}`;
    entry.innerHTML = `<strong>${title}</strong> — ${message}`;
    logPanel.prepend(entry);
    const limit = 40;
    while (logPanel.childElementCount > limit) {
      logPanel.removeChild(logPanel.lastElementChild);
    }
  }

  function setActionsDisabled(disabled) {
    if (actionForms.length === 0) return;
    actionForms.forEach((form) => {
      Array.from(form.elements).forEach((element) => {
        if (typeof element.disabled === "boolean" && element.type !== "submit") {
          element.disabled = disabled;
        }
      });
      const submit = form.querySelector("button[type=submit]");
      if (submit) submit.disabled = disabled;
    });
  }

  async function apiPost(url, body) {
    const response = await fetch(url, {
      method: "POST",
      headers: { "Content-Type": "application/json" },
      body: body ? JSON.stringify(body) : undefined
    });
    if (!response.ok) {
      let errorMessage = `Erro HTTP ${response.status}`;
      try {
        const payload = await response.json();
        if (payload?.error) errorMessage = payload.error;
      } catch {
        /* ignore */
      }
      throw new Error(errorMessage);
    }
    return response.json();
  }

  async function apiGet(url) {
    const response = await fetch(url);
    if (!response.ok) {
      throw new Error(`Erro HTTP ${response.status}`);
    }
    return response.json();
  }

  function buildActionCard(action) {
    const card = document.createElement("form");
    card.className = "action-card flex flex-col gap-3 rounded-2xl border border-slate-500/25 bg-slate-900/55 p-4";
    card.dataset.actionId = action.id;

    const heading = document.createElement("div");
    heading.className = "flex items-center justify-between gap-2";
    const title = document.createElement("h3");
    title.textContent = action.label;
    heading.appendChild(title);
    card.appendChild(heading);

    if (action.description) {
      const desc = document.createElement("p");
      desc.textContent = action.description;
      desc.style.opacity = "0.7";
      desc.style.fontSize = "0.85rem";
      card.appendChild(desc);
    }

    if (Array.isArray(action.parameters)) {
      action.parameters.forEach((parameter) => {
        const wrapper = document.createElement("div");
        const label = document.createElement("label");
        label.textContent = parameter.label;
        label.htmlFor = `${action.id}-${parameter.name}`;
        const input = document.createElement("input");
        input.id = `${action.id}-${parameter.name}`;
        input.name = parameter.name;
        input.placeholder = parameter.placeholder || "";
        input.type = parameter.type === "number" ? "number" : "text";
        input.required = true;
        input.className =
          "w-full rounded-xl border border-slate-500/40 bg-slate-900/70 px-3 py-2 text-slate-100 shadow-inner shadow-slate-900/20 focus:outline-none focus:ring-2 focus:ring-sky-400/60 focus:border-sky-400/60 transition";
        wrapper.appendChild(label);
        wrapper.appendChild(input);
        card.appendChild(wrapper);
      });
    }

    const submit = document.createElement("button");
    submit.type = "submit";
    submit.textContent = action.returnsValue ? "Consultar" : "Executar";
    submit.className =
      "px-4 py-2 rounded-full border border-slate-500/40 bg-slate-900/70 text-slate-100 font-semibold transition-transform hover:-translate-y-0.5 focus:outline-none focus:ring-2 focus:ring-slate-300/40 disabled:opacity-40 disabled:cursor-not-allowed";
    card.appendChild(submit);

    card.addEventListener("submit", async (event) => {
      event.preventDefault();
      if (!currentState.connected) {
        logActionResult("error", action.label, "Conecte-se ao dispositivo antes de executar ações.");
        return;
      }

      const formData = new FormData(card);
      const payload = {};
      let validationError = "";
      if (Array.isArray(action.parameters)) {
        action.parameters.forEach((parameter) => {
          const raw = formData.get(parameter.name);
          if (parameter.type === "number") {
            const value = Number(raw);
            if (!Number.isFinite(value)) {
              validationError = `${parameter.label} inválido`;
            } else {
              payload[parameter.name] = value;
            }
          } else {
            const value = String(raw ?? "").trim();
            if (!value) {
              validationError = `${parameter.label} não pode ficar vazio`;
            } else {
              payload[parameter.name] = value;
            }
          }
        });
      }

      if (validationError) {
        logActionResult("error", action.label, validationError);
        return;
      }

      try {
        const response = await apiPost("/api/action", { action: action.id, payload });
        if (response && Object.prototype.hasOwnProperty.call(response, "result")) {
          logActionResult("success", action.label, `Resultado: ${JSON.stringify(response.result)}`);
        } else {
          logActionResult("success", action.label, "Comando enviado");
        }
      } catch (error) {
        logActionResult("error", action.label, error.message || String(error));
      }
    });

    return card;
  }

  function renderActions(definitions) {
    if (!actionsContainer) return;
    actionsContainer.innerHTML = "";
    actionForms.length = 0;
    definitions.forEach((definition) => {
      const form = buildActionCard(definition);
      actionsContainer.appendChild(form);
      actionForms.push(form);
    });
    setActionsDisabled(!currentState.connected);
  }

  function setupWebSocket() {
    const protocol = window.location.protocol === "https:" ? "wss" : "ws";
    const wsUrl = `${protocol}://${window.location.host}/ws/stream`;
    let retry = 1000;

    function connect() {
      const socket = new WebSocket(wsUrl);

      socket.addEventListener("open", () => {
        retry = 1000;
      });

      socket.addEventListener("message", (event) => {
        try {
          const message = JSON.parse(event.data);
          if (message?.type === "state") {
            updateConnectionState(message.payload);
          }
          if (message?.type === "processData") {
            updateProcessData(message.payload);
          }
        } catch (error) {
          console.error("Erro ao interpretar mensagem WS", error);
        }
      });

      socket.addEventListener("close", () => {
        retry = Math.min(retry * 1.5, 10000);
        setTimeout(connect, retry);
      });

      socket.addEventListener("error", () => {
        socket.close();
      });
    }

    connect();
  }

  async function bootstrap() {
    try {
      if (actionsContainer) {
        const actionsResponse = await apiGet("/api/actions");
        actions = actionsResponse?.actions ?? [];
        renderActions(actions);
      }
      const state = await apiGet("/api/state");
      updateConnectionState(state);
      if (state?.snapshot) {
        updateProcessData(state.snapshot);
      }
    } catch (error) {
      logActionResult("error", "Inicialização", error.message || String(error));
    }

    connectBtn?.addEventListener("click", async () => {
      try {
        const state = await apiPost("/api/connect");
        updateConnectionState(state);
        logActionResult("success", "Conexão", "Conectado ao dispositivo");
      } catch (error) {
        logActionResult("error", "Conexão", error.message || String(error));
      }
    });

    disconnectBtn?.addEventListener("click", async () => {
      try {
        const state = await apiPost("/api/disconnect");
        updateConnectionState(state);
        logActionResult("info", "Conexão", "Desconectado do dispositivo");
      } catch (error) {
        logActionResult("error", "Conexão", error.message || String(error));
      }
    });

    setupWebSocket();
  }

  bootstrap();
})();
