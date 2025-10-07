#include "dse/device.hpp"

#include <chrono>
#include <iomanip>
#include <iostream>
#include <thread>

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <dse-ip-address>" << std::endl;
        return 1;
    }

    const std::string ip_address = argv[1];
    jetbus::JetBusClient::Options client_options;
    client_options.url = "ws://" + ip_address + ":80/jet/canopen";

    dse::Device::Options device_options;
    device_options.client_options = client_options;

    dse::Device device(device_options);

    device.set_process_data_callback([](const jetbus::ProcessData& process_data) {
        const auto& weight = process_data.weight();
        std::cout << std::fixed << std::setprecision(process_data.decimals())
                  << "[ProcessData] Net: " << weight.net
                  << " " << process_data.unit()
                  << " | Gross: " << weight.gross
                  << " | Tare: " << weight.tare
                  << " | Stable: " << std::boolalpha << process_data.weight_stable()
                  << std::noboolalpha << std::endl;
    });

    try {
        device.connect();
        std::this_thread::sleep_for(std::chrono::seconds(1));

        std::cout << "Connected to DSE device at " << ip_address << std::endl;
        std::cout << "Identification: " << device.identification() << std::endl;
        std::cout << "Firmware: " << device.firmware_version() << std::endl;
        std::cout << "Serial number: " << device.serial_number() << std::endl;

        std::cout << "Executing Zero command..." << std::endl;
        device.zero();
        std::cout << "Executing Tare command..." << std::endl;
        device.tare();
        std::cout << "Executing SetGross command..." << std::endl;
        device.set_gross();

        std::this_thread::sleep_for(std::chrono::seconds(2));
    } catch (const std::exception& ex) {
        std::cerr << "Error: " << ex.what() << std::endl;
        device.disconnect();
        return 1;
    }

    device.disconnect();
    return 0;
}
