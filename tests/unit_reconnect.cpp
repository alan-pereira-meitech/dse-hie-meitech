#include <gtest/gtest.h>

#include <chrono>
#include <thread>

#include "dsejet_ws_client.h"

TEST(Reconnect, BackoffIncreasesAfterFailures) {
    dsejet::Config config;
    config.host = "127.0.0.1";
    config.port = 1; // unreachable port
    config.tls = false;
    config.reconnect_initial = std::chrono::milliseconds(100);
    config.reconnect_max = std::chrono::milliseconds(800);
    config.ping_interval = std::chrono::milliseconds(500);
    config.request_timeout = std::chrono::milliseconds(200);
    config.connect_timeout = std::chrono::milliseconds(500);

    auto logger = std::make_shared<dsejet::Logger>(dsejet::LogLevel::Error);
    auto client = std::make_shared<dsejet::DSEJetWSClient>(config, logger);

    auto future = client->connect();
    EXPECT_THROW(future.get(), std::exception);

    std::this_thread::sleep_for(std::chrono::seconds(2));
    EXPECT_GE(client->current_backoff(), config.reconnect_initial * 2);

    client->close();
}

