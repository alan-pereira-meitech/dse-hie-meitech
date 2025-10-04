#include "dsejet/sdk.h"

#include <iostream>

int main(int argc, char **argv) {
    dsejet::Config config = dsejet::Config::FromEnv();
    config.ApplyCliArgs(argc, argv);

    dsejet::DSEJetSDK sdk(config);

    try {
        sdk.connect();
        std::cout << "Sending tare command..." << std::endl;
        sdk.tare();
        std::cout << "Sending zero command..." << std::endl;
        sdk.zero();
        std::cout << "Commands executed successfully." << std::endl;
    } catch (const std::exception &ex) {
        std::cerr << "Command failed: " << ex.what() << std::endl;
        sdk.disconnect();
        return 1;
    }

    sdk.disconnect();
    return 0;
}

