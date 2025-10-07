#include "dsemodbus/modbus_client.h"
#include <iostream>
#include <iomanip>
#include <cstdlib>
#include <string>
#include <chrono>
#include <thread>
#include <cstring>

using dsemodbus::ModbusClient;
using dsemodbus::ByteOrder;

// NOTE: Addresses and byte order below are assumptions based on earlier scans.
// Adjust these constants to match the device's register map when available.
namespace cfg {
    // Connection
    static std::string host()  { const char* v=getenv("MODBUS_HOST"); return v? v: std::string("192.168.1.243"); }
    static int port()          { const char* v=getenv("MODBUS_PORT"); return v? std::atoi(v): 502; }
    static int slave()         { const char* v=getenv("MODBUS_SLAVE"); return v? std::atoi(v): 1; }

    // Weight addresses (Input registers). Example: NET at 16, GROSS at 20
    static int addr_net()      { const char* v=getenv("MODBUS_ADDR_NET"); return v? std::atoi(v): 16; }
    static int addr_gross()    { const char* v=getenv("MODBUS_ADDR_GROSS"); return v? std::atoi(v): 20; }

    // Byte order: BE | LE | BE_SWAP | LE_SWAP
    static std::string byte_order() { const char* v=getenv("MODBUS_BYTEORDER"); return v? v: std::string("LE"); }

    // Polling interval
    static int interval_ms()   { const char* v=getenv("MODBUS_INTERVAL_MS"); return v? std::atoi(v): 200; }
}

static uint32_t assemble_u32(uint16_t r0, uint16_t r1, const std::string &ord) {
    if (ord == "BE")      return (uint32_t(r0) << 16) | r1;
    if (ord == "LE")      return (uint32_t(r1) << 16) | r0;
    if (ord == "BE_SWAP") { uint32_t a=(r0>>8)|((r0&0xFF)<<8); uint32_t b=(r1>>8)|((r1&0xFF)<<8); return (a<<16)|b; }
    if (ord == "LE_SWAP") { uint32_t a=(r0>>8)|((r0&0xFF)<<8); uint32_t b=(r1>>8)|((r1&0xFF)<<8); return (b<<16)|a; }
    return (uint32_t(r0) << 16) | r1;
}

static float u32_to_float(uint32_t u) { float f; std::memcpy(&f, &u, sizeof(f)); return f; }

static void maybe_configure_filters(ModbusClient &client) {
    // Hardcoded filter configuration placeholder.
    // Strategy for now:
    // 1) Read existing filter-related registers (addresses TBD) and print them.
    // 2) Optionally write back defaults (currently: do not change, just echo existing values).

    // TODO: Replace these addresses with the real ones once known.
    const int FILTER_ADDR_BASE = std::getenv("MODBUS_FILTER_BASE") ? std::atoi(std::getenv("MODBUS_FILTER_BASE")) : -1;
    const int FILTER_REGS_NB   = std::getenv("MODBUS_FILTER_NB") ? std::atoi(std::getenv("MODBUS_FILTER_NB")) : 0;

    if (FILTER_ADDR_BASE >= 0 && FILTER_REGS_NB > 0) {
        try {
            auto regs = client.read_holding_registers(FILTER_ADDR_BASE, FILTER_REGS_NB);
            std::cout << "[filters] holding[" << FILTER_ADDR_BASE << "]..[" << (FILTER_ADDR_BASE + (int)regs.size() - 1) << "]:\n";
            for (size_t i=0;i<regs.size();++i) {
                std::cout << "  R[" << (FILTER_ADDR_BASE + (int)i) << "] = " << regs[i] << "\n";
            }
            // If we want to hardcode default values later, we can write here via write_single_register/write_multiple_registers
        } catch(const std::exception &ex) {
            std::cerr << "[filters] read error: " << ex.what() << "\n";
        }
    } else {
        std::cout << "[filters] no filter block configured; skipping (set MODBUS_FILTER_BASE and MODBUS_FILTER_NB to enable)\n";
    }
}

int main() {
    try {
        ModbusClient client; client.connect_tcp(cfg::host(), cfg::port(), cfg::slave());
        std::cout << "Connected to " << cfg::host() << ":" << cfg::port() << " (slave=" << cfg::slave() << ")\n";

        // Read and echo current filters (no writes yet)
        maybe_configure_filters(client);

        std::string order = cfg::byte_order();
        uint32_t last_net_u = 0, last_gross_u = 0; bool have_net=false, have_gross=false;

        std::cout << "Streaming NET/GROSS from input registers NET@" << cfg::addr_net() << ", GROSS@" << cfg::addr_gross()
                  << " order=" << order << "\n";

        for(;;) {
            try {
                // Read NET
                auto net_regs = client.read_input_registers(cfg::addr_net(), 2);
                if (net_regs.size() == 2) {
                    uint32_t u = assemble_u32(net_regs[0], net_regs[1], order);
                    if (!have_net || u != last_net_u) {
                        last_net_u = u; have_net = true;
                        std::cout << std::fixed << std::setprecision(5)
                                  << "NET=" << u32_to_float(u);
                        if (!have_gross) std::cout << "\n";
                        else std::cout << " ";
                    }
                }

                // Read GROSS
                auto gross_regs = client.read_input_registers(cfg::addr_gross(), 2);
                if (gross_regs.size() == 2) {
                    uint32_t u = assemble_u32(gross_regs[0], gross_regs[1], order);
                    if (!have_gross || u != last_gross_u) {
                        last_gross_u = u; have_gross = true;
                        std::cout << std::fixed << std::setprecision(5)
                                  << "GROSS=" << u32_to_float(u) << "\n";
                    }
                }
            } catch (const std::exception &ex) {
                std::cerr << "Error: " << ex.what() << "\n";
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(cfg::interval_ms()));
        }
    } catch (const std::exception &ex) {
        std::cerr << "Error: " << ex.what() << "\n";
        return 1;
    }
    return 0;
}
