#include "dsejet/sdk.h"

#include <atomic>
#include <csignal>
#include <iostream>
#include <mutex>
#include <thread>

namespace {
std::atomic<bool> g_running{true};

void HandleSignal(int) {
    g_running = false;
}
} // namespace

int main(int argc, char **argv) {
    dsejet::Config config = dsejet::Config::FromEnv();
    config.ApplyCliArgs(argc, argv);

    dsejet::DSEJetSDK sdk(config);

    try {
        sdk.connect();
    } catch (const std::exception &ex) {
        std::cerr << "Failed to connect: " << ex.what() << std::endl;
        return 1;
    }

    std::cout << "Subscribed to Jet stream updates (Ctrl+C to exit)" << std::endl;

    std::mutex cout_mutex;
    sdk.subscribe_stream({"6144/00", "601A/01", "6012/01", "6002/02"},
                         [&cout_mutex](const std::string &path, const nlohmann::json &value) {
        std::lock_guard<std::mutex> lock(cout_mutex);
        std::cout << path << " => " << value.dump() << std::endl;
    });

    std::signal(SIGINT, HandleSignal);
    std::signal(SIGTERM, HandleSignal);

    while (g_running.load()) {
        std::this_thread::sleep_for(config.poll_interval);
    }

    sdk.disconnect();
    return 0;
}

