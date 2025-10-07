#include "dse/device.hpp"
#include <atomic>
#include <chrono>
#include <csignal>
#include <iomanip>
#include <iostream>
#include <thread>

static std::atomic<bool> g_run{true};

static void on_sigint(int) {
    g_run.store(false);
}

int main(int argc, char** argv) {
    if (argc < 2) { std::cerr << "Uso: " << argv[0] << " <ip-ou-url>\n"; return 1; }

    std::signal(SIGINT, on_sigint);

    const std::string arg = argv[1];
    jetbus::JetBusClient::Options client_options;
    client_options.enable_debug_logs = false;
    client_options.url = (arg.rfind("ws://", 0) == 0 || arg.rfind("wss://", 0) == 0)
                           ? arg
                           : ("ws://" + arg + "/jet/canopen");
    dse::Device device(dse::Device::Options{client_options});

    try {
    device.connect();
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

        while (g_run.load()) {
            const auto net = device.net_weight();
            const auto gross = device.gross_weight();
            const auto unit = device.unit();
            std::cout << std::fixed << std::setprecision(device.decimals())
                      << "Net: " << net << " " << unit
                      << " | Gross: " << gross
                      << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    } catch (const std::exception& ex) {
        std::cerr << "Erro: " << ex.what() << std::endl;
        device.disconnect();
        return 1;
    }

    device.disconnect();
    return 0;
}
