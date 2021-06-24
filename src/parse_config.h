#pragma once

#include <nlohmann/json.hpp>
#include <filesystem>
#include <fstream>


struct Config
{
    std::string address;
    unsigned int port;
    std::filesystem::path log;
};


inline Config parseConfig(const std::filesystem::path& path) {
    std::ifstream file{path};
    auto begin = std::istreambuf_iterator<char>(file);
    auto end = std::istreambuf_iterator<char>{};

    auto config = nlohmann::json::parse(begin, end);

    std::string address = config["address"];
    unsigned int port = config["port"];
    std::string log = config["log"];

    return Config{std::move(address), port, std::move(log)};
}

inline Config defaultConfig() {
    return Config{"127.0.0.1", 6789, "log.txt"};
}
