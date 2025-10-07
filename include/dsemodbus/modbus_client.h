#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <chrono>
#include <stdexcept>
#include <modbus/modbus.h>


namespace dsemodbus {

class ModbusError : public std::runtime_error {
public:
    explicit ModbusError(const std::string &msg) : std::runtime_error(msg) {}
};

enum class ByteOrder {
    BigEndian,      // Modbus default (ABCD)
    LittleEndian,   // DCBA
    BigEndianSwap,  // BADC
    LittleEndianSwap // CDAB
};

class ModbusClient {
public:
    ModbusClient();
    ~ModbusClient();

    // Non-copyable
    ModbusClient(const ModbusClient&) = delete;
    ModbusClient& operator=(const ModbusClient&) = delete;

    // Movable
    ModbusClient(ModbusClient&&) noexcept;
    ModbusClient& operator=(ModbusClient&&) noexcept;

    // TCP connect
    void connect_tcp(const std::string &host, int port = 502, int slave_id = 1,
                     std::chrono::milliseconds response_timeout = std::chrono::milliseconds(2000));
    void close();
    bool is_connected() const noexcept { return ctx_ != nullptr; }

    // Unit/slave id
    void set_slave(int slave_id);

    // Timeouts
    void set_response_timeout(std::chrono::milliseconds timeout);

    // Coils / Discrete Inputs
    std::vector<uint8_t> read_coils(int addr, int nb);
    std::vector<uint8_t> read_discrete_inputs(int addr, int nb);
    void write_single_coil(int addr, bool on);
    void write_multiple_coils(int addr, const std::vector<uint8_t> &bits);

    // Registers
    std::vector<uint16_t> read_holding_registers(int addr, int nb);
    std::vector<uint16_t> read_input_registers(int addr, int nb);
    void write_single_register(int addr, uint16_t value);
    void write_multiple_registers(int addr, const std::vector<uint16_t> &values);
    void mask_write_register(int addr, uint16_t and_mask, uint16_t or_mask);
    void read_write_registers(int read_addr, int read_nb, int write_addr, const std::vector<uint16_t> &write_vals,
                              std::vector<uint16_t> &read_back);

    // Helpers for 32-bit and float (using two 16-bit registers)
    uint32_t read_u32(int addr, ByteOrder order = ByteOrder::BigEndian);
    int32_t  read_i32(int addr, ByteOrder order = ByteOrder::BigEndian);
    float    read_float(int addr, ByteOrder order = ByteOrder::BigEndian);
    void     write_u32(int addr, uint32_t value, ByteOrder order = ByteOrder::BigEndian);
    void     write_i32(int addr, int32_t value, ByteOrder order = ByteOrder::BigEndian);
    void     write_float(int addr, float value, ByteOrder order = ByteOrder::BigEndian);

private:
    modbus_t *ctx_ = nullptr;

    static void reorder_u32(uint16_t hi, uint16_t lo, ByteOrder order, uint16_t &out0, uint16_t &out1);
    static void parse_u32(const uint16_t regs[2], ByteOrder order, uint32_t &uval);
    static void parse_i32(const uint16_t regs[2], ByteOrder order, int32_t &ival);
    static void parse_float(const uint16_t regs[2], ByteOrder order, float &fval);
};

} // namespace dsemodbus
