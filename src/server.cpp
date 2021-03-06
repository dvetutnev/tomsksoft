#include "server.h"
#include "parse_config.h"

#include <uvw.hpp>
#include <iostream>


int main(int argc, char* argv[]) {
    auto loop = uvw::Loop::getDefault();
    Config config = (argc > 1) ? parseConfig(argv[1]) : defaultConfig();

    std::cout << "Address: " << config.address << ", port: " << config.port << ", log: " << config.log << std::endl;

    Server server{*loop, config.address, config.port, config.log};

    auto signal = loop->resource<uvw::SignalHandle>();
    auto onSignal = [&server](const uvw::SignalEvent&, auto&) {
        server.stop();
    };
    signal->on<uvw::SignalEvent>(onSignal);
    signal->oneShot(SIGINT);


    loop->run();


    return EXIT_SUCCESS;
}
