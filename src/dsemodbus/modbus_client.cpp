#include "dsemodbus/modbus_client.h"

#include <modbus/modbus.h>
#include <cstring>

namespace dsemodbus {

static std::string last_error(modbus_t *ctx) {
    int err = errno;
    const char *str = modbus_strerror(err);
    return str ? std::string(str) : std::to_string(err);
}

ModbusClient::ModbusClient() = default;

ModbusClient::~ModbusClient() { close(); }

ModbusClient::ModbusClient(ModbusClient &&other) noexcept { ctx_ = other.ctx_; other.ctx_ = nullptr; }

ModbusClient &ModbusClient::operator=(ModbusClient &&other) noexcept {
    if (this != &other) {
        close();
        ctx_ = other.ctx_;
        other.ctx_ = nullptr;
    }
    return *this;
}

void ModbusClient::connect_tcp(const std::string &host, int port, int slave_id,
                               std::chrono::milliseconds response_timeout) {
    close();
    ctx_ = modbus_new_tcp(host.c_str(), port);
    if (!ctx_) {
        throw ModbusError("modbus_new_tcp failed");
    }
    if (modbus_connect(ctx_) == -1) {
        std::string err = last_error(ctx_);
    modbus_free(ctx_); ctx_ = nullptr;
        throw ModbusError(std::string("modbus_connect failed: ") + err);
    }
    set_slave(slave_id);
    set_response_timeout(response_timeout);
}

void ModbusClient::close() {
    if (ctx_) {
        modbus_close(ctx_);
        modbus_free(ctx_);
        ctx_ = nullptr;
    }
}

void ModbusClient::set_slave(int slave_id) {
    if (!ctx_) throw ModbusError("not connected");
    if (modbus_set_slave(ctx_, slave_id) == -1) {
        throw ModbusError(std::string("modbus_set_slave failed: ") + last_error(ctx_));
    }
}

void ModbusClient::set_response_timeout(std::chrono::milliseconds timeout) {
    if (!ctx_) throw ModbusError("not connected");
    struct timeval tv;
    tv.tv_sec = static_cast<time_t>(timeout.count() / 1000);
    tv.tv_usec = static_cast<suseconds_t>((timeout.count() % 1000) * 1000);
    if (modbus_set_response_timeout(ctx_, tv.tv_sec, tv.tv_usec) == -1) {
        throw ModbusError(std::string("modbus_set_response_timeout failed: ") + last_error(ctx_));
    }
}

std::vector<uint8_t> ModbusClient::read_coils(int addr, int nb) {
    if (!ctx_) throw ModbusError("not connected");
    std::vector<uint8_t> bits(nb);
    int rc = modbus_read_bits(ctx_, addr, nb, bits.data());
    if (rc == -1) throw ModbusError(last_error(ctx_));
    bits.resize(rc);
    return bits;
}

std::vector<uint8_t> ModbusClient::read_discrete_inputs(int addr, int nb) {
    if (!ctx_) throw ModbusError("not connected");
    std::vector<uint8_t> bits(nb);
    int rc = modbus_read_input_bits(ctx_, addr, nb, bits.data());
    if (rc == -1) throw ModbusError(last_error(ctx_));
    bits.resize(rc);
    return bits;
}

void ModbusClient::write_single_coil(int addr, bool on) {
    if (!ctx_) throw ModbusError("not connected");
    if (modbus_write_bit(ctx_, addr, on ? 1 : 0) == -1) throw ModbusError(last_error(ctx_));
}

void ModbusClient::write_multiple_coils(int addr, const std::vector<uint8_t> &bits) {
    if (!ctx_) throw ModbusError("not connected");
    if (modbus_write_bits(ctx_, addr, static_cast<int>(bits.size()), bits.data()) == -1) {
        throw ModbusError(last_error(ctx_));
    }
}

std::vector<uint16_t> ModbusClient::read_holding_registers(int addr, int nb) {
    if (!ctx_) throw ModbusError("not connected");
    std::vector<uint16_t> regs(nb);
    int rc = modbus_read_registers(ctx_, addr, nb, regs.data());
    if (rc == -1) throw ModbusError(last_error(ctx_));
    regs.resize(rc);
    return regs;
}

std::vector<uint16_t> ModbusClient::read_input_registers(int addr, int nb) {
    if (!ctx_) throw ModbusError("not connected");
    std::vector<uint16_t> regs(nb);
    int rc = modbus_read_input_registers(ctx_, addr, nb, regs.data());
    if (rc == -1) throw ModbusError(last_error(ctx_));
    regs.resize(rc);
    return regs;
}

void ModbusClient::write_single_register(int addr, uint16_t value) {
    if (!ctx_) throw ModbusError("not connected");
    if (modbus_write_register(ctx_, addr, value) == -1) throw ModbusError(last_error(ctx_));
}

void ModbusClient::write_multiple_registers(int addr, const std::vector<uint16_t> &values) {
    if (!ctx_) throw ModbusError("not connected");
    if (values.empty()) return;
    if (modbus_write_registers(ctx_, addr, static_cast<int>(values.size()), const_cast<uint16_t*>(values.data())) == -1) {
        throw ModbusError(last_error(ctx_));
    }
}

void ModbusClient::mask_write_register(int addr, uint16_t and_mask, uint16_t or_mask) {
    if (!ctx_) throw ModbusError("not connected");
    if (modbus_mask_write_register(ctx_, addr, and_mask, or_mask) == -1) throw ModbusError(last_error(ctx_));
}

void ModbusClient::read_write_registers(int read_addr, int read_nb, int write_addr, const std::vector<uint16_t> &write_vals,
                                        std::vector<uint16_t> &read_back) {
    if (!ctx_) throw ModbusError("not connected");
    read_back.resize(read_nb);
    int rc = modbus_write_and_read_registers(ctx_, write_addr, static_cast<int>(write_vals.size()), const_cast<uint16_t*>(write_vals.data()),
                                             read_addr, read_nb, read_back.data());
    if (rc == -1) throw ModbusError(last_error(ctx_));
    read_back.resize(rc);
}

void ModbusClient::reorder_u32(uint16_t hi, uint16_t lo, ByteOrder order, uint16_t &out0, uint16_t &out1) {
    switch (order) {
        case ByteOrder::BigEndian:        out0 = hi; out1 = lo; break;      // ABCD
        case ByteOrder::LittleEndian:     out0 = lo; out1 = hi; break;      // DCBA
        case ByteOrder::BigEndianSwap:    out0 = (uint16_t)((hi << 8) | (hi >> 8)); out1 = (uint16_t)((lo << 8) | (lo >> 8)); break; // BADC
        case ByteOrder::LittleEndianSwap: out0 = (uint16_t)((lo << 8) | (lo >> 8)); out1 = (uint16_t)((hi << 8) | (hi >> 8)); break; // CDAB
    }
}

void ModbusClient::parse_u32(const uint16_t regs[2], ByteOrder order, uint32_t &uval) {
    uint16_t r0 = regs[0], r1 = regs[1];
    switch (order) {
        case ByteOrder::BigEndian:        uval = ((uint32_t)r0 << 16) | r1; break;
        case ByteOrder::LittleEndian:     uval = ((uint32_t)r1 << 16) | r0; break;
        case ByteOrder::BigEndianSwap:    uval = ((uint32_t)((r0 << 8) | (r0 >> 8)) << 16) | ((r1 << 8) | (r1 >> 8)); break;
        case ByteOrder::LittleEndianSwap: uval = ((uint32_t)((r1 << 8) | (r1 >> 8)) << 16) | ((r0 << 8) | (r0 >> 8)); break;
    }
}

void ModbusClient::parse_i32(const uint16_t regs[2], ByteOrder order, int32_t &ival) {
    uint32_t uval = 0; parse_u32(regs, order, uval); ival = (int32_t)uval;
}

void ModbusClient::parse_float(const uint16_t regs[2], ByteOrder order, float &fval) {
    uint32_t u = 0; parse_u32(regs, order, u); std::memcpy(&fval, &u, sizeof(float));
}

uint32_t ModbusClient::read_u32(int addr, ByteOrder order) {
    auto regs = read_holding_registers(addr, 2);
    if (regs.size() < 2) throw ModbusError("read_u32: not enough data");
    uint32_t u = 0; parse_u32(regs.data(), order, u); return u;
}

int32_t ModbusClient::read_i32(int addr, ByteOrder order) {
    auto regs = read_holding_registers(addr, 2);
    if (regs.size() < 2) throw ModbusError("read_i32: not enough data");
    int32_t i = 0; parse_i32(regs.data(), order, i); return i;
}

float ModbusClient::read_float(int addr, ByteOrder order) {
    auto regs = read_holding_registers(addr, 2);
    if (regs.size() < 2) throw ModbusError("read_float: not enough data");
    float f = 0.f; parse_float(regs.data(), order, f); return f;
}

void ModbusClient::write_u32(int addr, uint32_t value, ByteOrder order) {
    uint16_t hi = (uint16_t)(value >> 16), lo = (uint16_t)(value & 0xFFFF);
    uint16_t out0, out1; reorder_u32(hi, lo, order, out0, out1);
    std::vector<uint16_t> regs{out0, out1};
    write_multiple_registers(addr, regs);
}

void ModbusClient::write_i32(int addr, int32_t value, ByteOrder order) {
    write_u32(addr, (uint32_t)value, order);
}

void ModbusClient::write_float(int addr, float value, ByteOrder order) {
    uint32_t u; std::memcpy(&u, &value, sizeof(float));
    write_u32(addr, u, order);
}

} // namespace dsemodbus
