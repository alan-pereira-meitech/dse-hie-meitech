#include "dsejet/sdk.h"

#include <chrono>
#include <iostream>
#include <thread>

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

    std::cout << "Polling Jet values every " << config.poll_interval.count() << " ms" << std::endl;

    const std::vector<std::string> paths = {"6144/00", "601A/01", "6012/01"};

    while (true) {
        for (const auto &path : paths) {
            try {
                auto value = sdk.read_once(path, config.request_timeout);
                std::cout << path << " => " << value.dump() << std::endl;
            } catch (const std::exception &ex) {
                std::cerr << "Read error for " << path << ": " << ex.what() << std::endl;
            }
        }
        std::this_thread::sleep_for(config.poll_interval);
    }

    return 0;
}

