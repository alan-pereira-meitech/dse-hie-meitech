#include <chrono>
#include <iostream>
#include <map>
#include <thread>

#include "hbm/automation/api/weighing/wtx/wtx_jet.hpp"

using namespace hbm::automation::api;

int main()
{
    auto connection = std::make_shared<weighing::wtx::jet::JetBusConnection>("127.0.0.1");
    int counter = 0;

    connection->set_fetch_function([&]() {
        std::map<std::string, std::string> data;
        using Commands = weighing::wtx::jet::JetBusCommands;
        data[Commands::CIA461Decimals().path()] = "2";
        data[Commands::CIA461NetValue().path()] = std::to_string(1230 + counter * 5);
        data[Commands::CIA461GrossValue().path()] = std::to_string(1500 + counter * 5);
        data[Commands::CIA461TareValue().path()] = "200";
        data[Commands::CIA461Unit().path()] = std::to_string(0x00020000);
        data[Commands::IMDApplicationMode().path()] = "0";
        data[Commands::CIA461WeightStatusGeneralWeightError().path()] = "0";
        data[Commands::CIA461WeightStatusLimitStatus().path()] = "0";
        data[Commands::CIA461WeightStatusManualTare().path()] = "0";
        data[Commands::CIA461WeightStatusWeightType().path()] = "0";
        data[Commands::CIA461WeightStatusWeightMoving().path()] = "0";
        data[Commands::CIA461WeightStatusScaleSealIsOpen().path()] = "0";
        data[Commands::CIA461WeightStatusScaleRange().path()] = "0";
        data[Commands::CIA461WeightStatusZeroRequired().path()] = "0";
        data[Commands::CIA461WeightStatusCenterOfZero().path()] = "1";
        data[Commands::CIA461WeightStatusInsideZero().path()] = "1";
        ++counter;
        return data;
    });

    connection->connect();

    weighing::wtx::WTXJet device(connection, 500, [](const weighing::ProcessDataReceivedEventArgs& args) {
        const auto* jet_data = dynamic_cast<const data::JetProcessData*>(&args.process_data);
        if (jet_data != nullptr) {
            std::cout << "Net weight: " << jet_data->weight().net << " " << jet_data->unit() << std::endl;
        }
    });

    device.start();
    std::this_thread::sleep_for(std::chrono::seconds(2));
    device.stop();

    connection->disconnect();
    return 0;
}
