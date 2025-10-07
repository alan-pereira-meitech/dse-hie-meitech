# Modbus TCP Client (C++)

This adds a simple C++ Modbus TCP client using libmodbus.

## Build

Requirements:
- libmodbus (Ubuntu/Debian: `sudo apt install libmodbus-dev`)
- g++ (C++17)

Example compile (standalone):

```
g++ -std=c++17 -Iinclude -o build/bin/modbus_demo examples/modbus_demo.cpp src/dsemodbus/modbus_client.cpp -lmodbus
```

If you already use CMake in this repo, I can wire targets; otherwise, use the line above as a quick start.

## Run

```
MODBUS_HOST=192.168.1.243 MODBUS_PORT=502 MODBUS_SLAVE=1 MODBUS_ADDR=0 MODBUS_NB=8 ./build/bin/modbus_demo
```

It will:
- Connect to MODBUS_HOST:MODBUS_PORT with the given slave/unit id
- Read MODBUS_NB holding registers from MODBUS_ADDR and print them
- Try to also parse a float at MODBUS_ADDR (big-endian word order)

## API

`include/dsemodbus/modbus_client.h` provides a `ModbusClient` class with:
- connect_tcp/close, set_slave, set_response_timeout
- read_coils, read_discrete_inputs, write_single_coil, write_multiple_coils
- read_holding_registers, read_input_registers, write_single_register, write_multiple_registers
- mask_write_register, read_write_registers
- read_u32/read_i32/read_float and corresponding write helpers with selectable byte order

If you want this integrated into your existing build system (CMake/Make), tell me which you prefer and I’ll add the targets.
