#include <gtest/gtest.h>

#include "dsejet_ws_client.h"

TEST(MessageCodec, AuthenticateRequestHasCredentials) {
    auto request = dsejet::MakeAuthenticateRequest(1, "user", "pass");
    EXPECT_EQ(request.at("type"), "call");
    EXPECT_EQ(request.at("id"), 1);
    EXPECT_EQ(request.at("method"), "authenticate");
    ASSERT_TRUE(request.contains("params"));
    EXPECT_EQ(request.at("params").at("user"), "user");
    EXPECT_EQ(request.at("params").at("password"), "pass");
}

TEST(MessageCodec, FetchRequestEncodesSubscription) {
    auto request = dsejet::MakeFetchRequest(2, "6144/00", true);
    EXPECT_EQ(request.at("path"), "6144/00");
    EXPECT_TRUE(request.at("params").at("subscribe"));
}

TEST(MessageCodec, ParseEvent) {
    nlohmann::json message{{"type", "event"}, {"path", "6144/00"}, {"event", "change"}, {"value", 42}};
    dsejet::JetEvent event;
    ASSERT_TRUE(dsejet::ParseJetEvent(message, event));
    EXPECT_EQ(event.path, "6144/00");
    EXPECT_EQ(event.event, "change");
    EXPECT_EQ(event.value, 42);
}

