#pragma once

#include "session.h"
#include "writer.h"

#include <uvw.hpp>

#include <set>
#include <filesystem>
#include <algorithm>
#include <iostream>
#include <cassert>


class Server
{
public:
    Server(uvw::Loop& loop, std::string ip, unsigned int port, const std::filesystem::path&);

    void remove(SessionBase*);
    void stop();

private:
    uvw::Loop& loop;

    std::shared_ptr<uvw::TCPHandle> listener;

    using W = Writer<uvw::FileReq>;
    std::shared_ptr<W> writer;

    using S = Session<Server, W, uvw::TCPHandle, uvw::TimerHandle>;
    std::set<std::shared_ptr<SessionBase>> connections;
};


inline Server::Server(uvw::Loop& loop, std::string ip, unsigned int port, const std::filesystem::path& path)
    :
      loop{loop}
{
    listener = loop.resource<uvw::TCPHandle>();

    auto onConnect = [this] (const uvw::ListenEvent&, auto&) {
        auto client = this->loop.resource<uvw::TCPHandle>();
        this->listener->accept(*client);

        auto timer = this->loop.resource<uvw::TimerHandle>();

        auto session = std::make_shared<S>(*this, *writer, std::move(client), std::move(timer));
        this->connections.insert(std::move(session));
    };
    listener->on<uvw::ListenEvent>(onConnect);

    auto onError = [] (const uvw::ErrorEvent& e, auto&) {
        std::cout << "Server error: " << e.what() << std::endl;
        std::abort();
    };
    listener->on<uvw::ErrorEvent>(onError);

    listener->bind("127.0.0.1", 4242);
    listener->listen();

    auto file = loop.resource<uvw::FileReq>();
    file->openSync(path, O_CREAT | O_RDWR, 0644);
    writer = std::make_shared<W>(std::move(file));
}

inline void Server::remove(SessionBase* s) {
    auto pred = [s] (const std::shared_ptr<SessionBase>& conn) -> bool {
        return conn.get() == s;
    };
    auto it = std::find_if(std::begin(connections), std::end(connections), pred);
    assert(it != std::end(connections));
    connections.erase(it);
}

inline void Server::stop() {
    listener->close();

    auto copy = connections;
    for (auto& session : copy) {
        session->halt();
    }

    std::cout << "Server in stop" << std::endl;
}
