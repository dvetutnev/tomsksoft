#include <uvw.hpp>
#include <gmock/gmock.h>


using ::testing::InSequence;


namespace {


struct Mock
{
    MOCK_METHOD(void, read, (), ());
    MOCK_METHOD(void, disconnect, (), ());
};


} // Anonymous namespace


TEST(uvw, shutdown) {
    Mock mock;
    {
        InSequence _;
        EXPECT_CALL(mock, read);
        EXPECT_CALL(mock, disconnect);
    }

    auto onError = [] (const uvw::ErrorEvent& e, auto&) {
        FAIL() << e.what();
    };


    auto loop = uvw::Loop::getDefault();


    auto listener = loop->resource<uvw::TCPHandle>();

    auto sessionOnRead = [&mock] (const uvw::DataEvent&, auto&) {
        mock.read();
    };
    auto sessionOnDisconnect = [&mock, &listener] (const uvw::EndEvent&, uvw::TCPHandle& session) {
        mock.disconnect();
        session.close();
        listener->close();
    };
    auto listenerOnConnect = [&loop, &listener, &sessionOnRead, &sessionOnDisconnect, &onError] (const uvw::ListenEvent&, auto&) {
        auto session = loop->resource<uvw::TCPHandle>();
        session->on<uvw::DataEvent>(sessionOnRead);
        session->on<uvw::EndEvent>(sessionOnDisconnect);
        session->on<uvw::ErrorEvent>(onError);

        listener->accept(*session);
        session->read();
    };

    listener->on<uvw::ListenEvent>(listenerOnConnect);
    listener->on<uvw::ErrorEvent>(onError);

    listener->bind("127.0.0.1", 6789);
    listener->listen();


    auto client = loop->resource<uvw::TCPHandle>();

    auto clientOnConnect = [&client] (const uvw::ConnectEvent&, auto&) {
        std::string d = "asdfghjkl";
        client->write(d.data(), d.length());
    };
    auto clientOnWrite = [&client] (const uvw::WriteEvent&, auto&) {
        client->shutdown();
    };
    auto clientOnShutdown = [&client] (const uvw::ShutdownEvent&, auto&) {
        client->close();
    };

    client->on<uvw::ConnectEvent>(clientOnConnect);
    client->on<uvw::WriteEvent>(clientOnWrite);
    client->on<uvw::ShutdownEvent>(clientOnShutdown);

    client->connect("127.0.0.1", 6789);

    loop->run();
}
