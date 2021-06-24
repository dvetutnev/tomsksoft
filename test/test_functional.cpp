#include "server.h"
#include "create_packet.h"

#include <uvw.hpp>
#include <gtest/gtest.h>

#include <fstream>


TEST(Functional, _) {
    const std::filesystem::path path = "log.txt";
    if (std::filesystem::exists(path)) {
        std::filesystem::remove(path);
    }

    auto loop = uvw::Loop::getDefault();

    Server server{*loop, "127.0.0.1", 4242, path};

    auto socket = loop->resource<uvw::TCPHandle>();
    auto onConnect = [](const uvw::ConnectEvent&, uvw::TCPHandle& socket){
        uvw::DataEvent packet = createPacket("sTRINg");
        socket.write(std::move(std::move(packet.data)), packet.length);
        socket.close();
    };
    auto onError = [](const uvw::ErrorEvent& e, auto&) {
        FAIL() << "Connect failed: " << e.what();
    };

    socket->once<uvw::ConnectEvent>(onConnect);
    socket->on<uvw::ErrorEvent>(onError);
    socket->connect(std::string{"127.0.0.1"}, 4242);

    auto timer = loop->resource<uvw::TimerHandle>();
    auto onTimer = [&server, &timer](const uvw::TimerEvent&, auto&) {
        server.stop();
        timer->close();
    };
    timer->on<uvw::TimerEvent>(onTimer);

    timer->start(std::chrono::milliseconds{100}, std::chrono::milliseconds{0});


    loop->run();


    std::fstream file{path};
    std::string log;
    file >> log;

    ASSERT_EQ(log, "sTRINg");
}
