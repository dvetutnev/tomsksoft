#pragma once

#include <boost/sml.hpp>
#include <uvw.hpp>
#include <arpa/inet.h> // ntohl

#include <memory>
#include <chrono>


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


    std::string headerBuffer;
    std::uint32_t dataLength;
    std::string dataBuffer;

    void init();
    void pushToHeaderBuffer(char);
    bool isHeaderComplete() const;
    void processHeader();
    void pushToDataBuffer(char);
    bool isDataComplete() const;
    void processResult();
    void restart();
    void halt();

    struct DefFSM
    {
        explicit DefFSM(Session& s) : session{s} {}

        struct InitEvent {};
        struct ByteEvent { char byte; };
        struct HaltEvent {};

        auto operator()() const {

            auto init = [this] () { session.init(); };
            auto pushToHeaderBuffer = [this] (const ByteEvent& e) { session.pushToHeaderBuffer(e.byte); };
            auto isHeaderComplete = [this] () -> bool { return session.isHeaderComplete(); };
            auto processHeader = [this] () { session.processHeader(); };
            auto pushToDataBuffer = [this] (const ByteEvent& e) { session.pushToDataBuffer(e.byte); };
            auto isDataComplete = [this] () -> bool { return session.isDataComplete(); };
            auto processResult = [this] { session.processResult(); };
            auto restart = [this] { session.restart(); };
            auto halt = [this] { session.halt(); };

            using namespace boost::sml;

            return make_transition_table(
                       *"Wait init"_s      + event<InitEvent> / init               = "Wait header"_s,
                        "Receive header"_s + event<ByteEvent> / pushToHeaderBuffer = "Is header complete"_s,

                       *"In work"_s        + event<HaltEvent> / halt               = X
            );
        }

        Session& session;
    };

    DefFSM defFsm;
    boost::sml::sm<DefFSM> fsm;
};


template <typename Server, typename Writer, typename Socket, typename Timer>
Session<Server, Writer, Socket, Timer>::Session(Server& s, Writer& w, std::shared_ptr<Socket> c, std::shared_ptr<Timer> t)
    :
      server{s},
      writer{w},

      client{std::move(c)},
      timer{std::move(t)},

      defFsm{*this},
      fsm{defFsm}
{
    auto dataHandler = [this] (const uvw::DataEvent&, auto&) {};
    auto errorHandler = [this] (const uvw::ErrorEvent&, auto&) { fsm.process_event(typename DefFSM::HaltEvent{}); };

    client->template on<uvw::DataEvent>(dataHandler);
    client->template on<uvw::ErrorEvent>(errorHandler);


    auto timeoutHandler = [this] (const uvw::TimerEvent&, auto&) { fsm.process_event(typename DefFSM::HaltEvent{}); };

    timer->template on<uvw::TimerEvent>(timeoutHandler);


    fsm.process_event(typename DefFSM::InitEvent{});
}


template <typename Server, typename Writer, typename Socket, typename Timer>
void Session<Server, Writer, Socket, Timer>::init() {
    client->read();
    timer->start(std::chrono::seconds{20}, std::chrono::seconds{20});
}

template <typename Server, typename Writer, typename Socket, typename Timer>
void Session<Server, Writer, Socket, Timer>::pushToHeaderBuffer(char c) {
    headerBuffer.push_back(c);
};

template <typename Server, typename Writer, typename Socket, typename Timer>
bool Session<Server, Writer, Socket, Timer>::isHeaderComplete() const {
    return headerBuffer.size() == 4;
};

template <typename Server, typename Writer, typename Socket, typename Timer>
void Session<Server, Writer, Socket, Timer>::processHeader() {
    dataLength = ::ntohl(*(reinterpret_cast<std::uint32_t*>(headerBuffer.data())));
};

template <typename Server, typename Writer, typename Socket, typename Timer>
void Session<Server, Writer, Socket, Timer>::pushToDataBuffer(char c) {
    dataBuffer.push_back(c);
};

template <typename Server, typename Writer, typename Socket, typename Timer>
bool Session<Server, Writer, Socket, Timer>::isDataComplete() const {
    return dataBuffer.size() == dataLength;
};

template <typename Server, typename Writer, typename Socket, typename Timer>
void Session<Server, Writer, Socket, Timer>::processResult() {
    writer.push(std::move(dataBuffer));
};

template <typename Server, typename Writer, typename Socket, typename Timer>
void Session<Server, Writer, Socket, Timer>::restart() {
    headerBuffer.clear();
    dataBuffer.clear();

    timer->again();
};

template <typename Server, typename Writer, typename Socket, typename Timer>
void Session<Server, Writer, Socket, Timer>::halt() {
    client->stop();
    client->close();

    timer->stop();
    timer->close();

    server.remove(this);
};
