#pragma once

#include <memory>
#include <boost/sml.hpp>
#include <uvw.hpp>


class SessionBase
{
public:
    virtual ~SessionBase() = default;
};


template <typename Server, typename Writer, typename Socket, typename Timer>
class Session : public SessionBase
{
public:
    Session(Server&, Writer&, std::shared_ptr<Socket>, std::shared_ptr<Timer>);

private:
    Server& server;
    Writer& writer;

    std::shared_ptr<Socket> client;
    std::shared_ptr<Timer> timer;
};


template <typename Server, typename Writer, typename Socket, typename Timer>
Session<Server, Writer, Socket, Timer>::Session(Server& s, Writer& w, std::shared_ptr<Socket> c, std::shared_ptr<Timer> t)
    :
      server{s},
      writer{w},

      client{std::move(c)},
      timer{std::move(t)}
{
    auto dataHandler = [] (const uvw::DataEvent&, auto&) {};
    auto errorHandler = [] (const uvw::ErrorEvent&, auto&) {};

    client->template on<uvw::DataEvent>(dataHandler);
    client->template on<uvw::ErrorEvent>(errorHandler);

    auto timeoutHandler = [] (const uvw::TimerEvent&, auto&) {};

    timer->template on<uvw::TimerEvent>(timeoutHandler);
}
