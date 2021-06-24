#include "parse_config.h"

#include <gtest/gtest.h>
#include <fstream>


TEST(Config, unexistsFile) {
    ASSERT_ANY_THROW(parseConfig("unexistConfig.json"));
}


TEST(Config, notAdderss) {
    std::string content = R"(
      {
        "port": 4543,
        "log": "/tmp/tomsk-server.log"
      }
    )";

    std::ofstream{"config.json"} << content;

    ASSERT_ANY_THROW(parseConfig("config.json"));
}

TEST(Config, notPort) {
    std::string content = R"(
      {
        "address": "example.org",
        "log": "/tmp/tomsk-server.log"
      }
    )";

    std::ofstream{"config.json"} << content;

    ASSERT_ANY_THROW(parseConfig("config.json"));
}

TEST(Config, portNotNumber) {
    std::string content = R"(
      {
        "address": "example.org",
        "port": "not_nu,ber",
        "log": "/tmp/tomsk-server.log"
      }
    )";

    std::ofstream{"config.json"} << content;

    ASSERT_ANY_THROW(parseConfig("config.json"));
}

TEST(Config, notLog) {
    std::string content = R"(
      {
        "address": "example.org",
        "port": 1510,
      }
    )";

    std::ofstream{"config.json"} << content;

    ASSERT_ANY_THROW(parseConfig("config.json"));
}

TEST(Config, normal) {
    std::string content = R"(
      {
        "address": "example.org",
        "port": 4224,
        "log": "/tmp/tomsk-server.log"
      }
    )";

    std::ofstream{"config.json"} << content;

    Config config = parseConfig("config.json");

    EXPECT_EQ(config.address, "example.org");
    EXPECT_EQ(config.port, 4224);
    EXPECT_EQ(config.log, "/tmp/tomsk-server.log");
}
