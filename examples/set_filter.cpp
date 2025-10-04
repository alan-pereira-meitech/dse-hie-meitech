#include "dsejet/sdk.h"

#include <iostream>
#include <stdexcept>

int main(int argc, char **argv) {
    dsejet::Config config = dsejet::Config::FromEnv();
    config.ApplyCliArgs(argc, argv);

    int stage = 2;
    std::string type = "fir_comb";
    int frequency = 10;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        auto next_value = [&](const char *flag) {
            if (i + 1 >= argc) {
                throw std::invalid_argument(std::string("Missing value for ") + flag);
            }
            return std::string(argv[++i]);
        };
        if (arg == "--stage") {
            stage = std::stoi(next_value("--stage"));
        } else if (arg == "--type") {
            type = next_value("--type");
        } else if (arg == "--frequency") {
            frequency = std::stoi(next_value("--frequency"));
        }
    }

    dsejet::DSEJetSDK sdk(config);

    try {
        sdk.connect();
        std::cout << "Setting stage " << stage << " filter to '" << type << "' with frequency " << frequency << std::endl;
        sdk.set_filter_stage(stage, type, frequency);
        std::cout << "Filter updated." << std::endl;
    } catch (const std::exception &ex) {
        std::cerr << "Failed to set filter: " << ex.what() << std::endl;
        sdk.disconnect();
        return 1;
    }

    sdk.disconnect();
    return 0;
}

