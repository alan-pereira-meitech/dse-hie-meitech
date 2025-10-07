#include <cassert>
#include <cmath>
#include <condition_variable>
#include <map>
#include <mutex>
#include <thread>

#include "hbm/automation/api/weighing/wtx/wtx_jet.hpp"

using namespace hbm::automation::api;

int main()
{
    auto connection = std::make_shared<weighing::wtx::jet::JetBusConnection>("127.0.0.1");

    connection->set_fetch_function([]() {
        std::map<std::string, std::string> data;
        using Commands = weighing::wtx::jet::JetBusCommands;
        data[Commands::CIA461Decimals().path()] = "2";
        data[Commands::CIA461NetValue().path()] = "1234";
        data[Commands::CIA461GrossValue().path()] = "1500";
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
        return data;
    });

    connection->connect();

    std::mutex mutex;
    std::condition_variable cv;
    bool received = false;
    double net_value = 0.0;

    weighing::wtx::WTXJet device(connection, 100, [&](const weighing::ProcessDataReceivedEventArgs& args) {
        const auto* jet_data = dynamic_cast<const data::JetProcessData*>(&args.process_data);
        if (jet_data != nullptr) {
            std::lock_guard<std::mutex> lock(mutex);
            net_value = jet_data->weight().net;
            received = true;
            cv.notify_all();
        }
    });

    device.start();

    std::unique_lock<std::mutex> lock(mutex);
    if (!cv.wait_for(lock, std::chrono::milliseconds(500), [&]() { return received; })) {
        return 1;
    }

    device.stop();
    connection->disconnect();

    assert(std::abs(net_value - 12.34) < 1e-6);
    return 0;
}
