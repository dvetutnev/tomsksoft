#include "create_packet.h"
#include "uvw.hpp"

#include <iostream>
#include <tuple>
#include <optional>


auto parseCmd(int argc, char* argv[]) {
    if (argc < 4) {
        std::cerr << "client 127.0.0.1 242 daaaaaaTa Dats222222" << std::endl;
        std::abort();
    }

    std::string address = argv[1];
    unsigned int port = std::stoul(argv[2]);

    std::vector<std::string> data;
    for (std::size_t i  = 3; i < argc; i++) {
        data.push_back(argv[i]);
    }

    return std::make_tuple(std::move(address), port, std::move(data));
}


int main(int argc, char* argv[]) {
    auto [address, port, data] = parseCmd(argc, argv);
    auto getData = [it = std::begin(data), end = std::end(data)] () mutable -> std::optional<uvw::DataEvent> {
        if (it != end) {
            const std::string d = *it;
            uvw::DataEvent packet = createPacket(d);
            ++it;
            return packet;
        }
        else {
            return std::nullopt;
        }
    };

    auto loop = uvw::Loop::getDefault();
    auto client = loop->resource<uvw::TCPHandle>();

    auto clientOnConnect = [&client, &getData] (const uvw::ConnectEvent&, auto&) {
        auto d = getData();
        client->write(std::move(d->data), d->length);
    };
    auto clientOnWrite = [&client, &getData] (const uvw::WriteEvent&, auto&) {
        auto d = getData();
        if (d) {
            client->write(std::move(d->data), d->length);
        }
        else {
            client->shutdown();
        }
    };
    auto clientOnShutdown = [&client] (const uvw::ShutdownEvent&, auto&) {
        client->close();
    };
    auto onError = [] (const uvw::ErrorEvent& e, auto&) {
        std::cerr << "onError: " << e.what() << std::endl;
        std::abort();
    };

    client->on<uvw::ConnectEvent>(clientOnConnect);
    client->on<uvw::WriteEvent>(clientOnWrite);
    client->on<uvw::ShutdownEvent>(clientOnShutdown);
    client->on<uvw::ErrorEvent>(onError);

    client->connect(address, port);

    loop->run();

    return EXIT_SUCCESS;
}
