#include <gtest/gtest.h>

#include <cstdlib>
#include <string>

#include "dsejet/sdk.h"

TEST(Integration, EndToEndReadWrite) {
    const char *flag = std::getenv("INTEGRATION_DSE");
    if (!flag || std::string(flag) != "1") {
        GTEST_SKIP() << "INTEGRATION_DSE not enabled";
    }

    dsejet::Config config = dsejet::Config::FromEnv();
    config.connect_timeout = std::chrono::milliseconds(10000);

    dsejet::DSEJetSDK sdk(config);

    ASSERT_NO_THROW(sdk.connect());

    auto gross = sdk.read_once("6144/00", config.request_timeout);
    (void)gross;

    sdk.disconnect();
}

