#include "session.h"
#include "mocks.h"
#include "create_packet.h"

#include <algorithm>


using ::testing::SaveArg;
using ::testing::InSequence;
using ::testing::StrictMock;
using ::testing::NiceMock;
using ::testing::AtLeast;


TEST(Session, timeout) {
    MockServer server;
    StrictMock<MockWriter> writer;

    auto client = std::make_shared<NiceMock<MockSocket>>();

    MockHandle::THandler<uvw::DataEvent> handlerDataEvent;
    EXPECT_CALL(*client, saveDataHandler).WillOnce(SaveArg<0>(&handlerDataEvent));
    {
        InSequence _;
        EXPECT_CALL(*client, read).Times(1);
        EXPECT_CALL(*client, stop).Times(1);
        EXPECT_CALL(*client, close).Times(1);
    }

    auto timer = std::make_shared<StrictMock<MockTimer>>();

    MockHandle::THandler<uvw::TimerEvent> handlerTimerEvent;
    EXPECT_CALL(*timer, saveHandler).WillOnce(SaveArg<0>(&handlerTimerEvent));
    {
        InSequence _;
        EXPECT_CALL(*timer, start).Times(1);
        EXPECT_CALL(*timer, stop).Times(1);
        EXPECT_CALL(*timer, close).Times(1);
    }

    Session<MockServer, MockWriter, MockSocket, MockTimer> session{server, writer, client, timer};

    EXPECT_CALL(server, remove(&session)).Times(1);

    uvw::DataEvent dataEvent{std::make_unique<char[]>(3), 3};;
    handlerDataEvent(dataEvent, *client);
    handlerTimerEvent(uvw::TimerEvent{}, *timer);
}

TEST(Session, networkError) {
    MockServer server;
    StrictMock<MockWriter> writer;

    auto client = std::make_shared<NiceMock<MockSocket>>();

    MockHandle::THandler<uvw::DataEvent> handlerDataEvent;
    EXPECT_CALL(*client, saveDataHandler).WillOnce(SaveArg<0>(&handlerDataEvent));

    MockHandle::THandler<uvw::ErrorEvent> handlerErrorEvent;
    EXPECT_CALL(*client, saveErrorHandler).WillOnce(SaveArg<0>(&handlerErrorEvent));
    {
        InSequence _;
        EXPECT_CALL(*client, read).Times(1);
        EXPECT_CALL(*client, stop).Times(1);
        EXPECT_CALL(*client, close).Times(1);
    }

    auto timer = std::make_shared<NiceMock<MockTimer>>();
    {
        InSequence _;
        EXPECT_CALL(*timer, start).Times(1);
        EXPECT_CALL(*timer, stop).Times(1);
        EXPECT_CALL(*timer, close).Times(1);
    }

    Session<MockServer, MockWriter, MockSocket, MockTimer> session{server, writer, client, timer};

    EXPECT_CALL(server, remove(&session)).Times(1);

    uvw::DataEvent dataEvent{std::make_unique<char[]>(3), 3};;
    handlerDataEvent(dataEvent, *client);
    handlerErrorEvent(uvw::ErrorEvent{static_cast<std::underlying_type_t<uv_errno_t>>(UV_EFAULT)}, *client);
}

TEST(Session, normal) {
    StrictMock<MockServer> server;
    MockWriter writer;

    auto client = std::make_shared<NiceMock<MockSocket>>();

    MockHandle::THandler<uvw::DataEvent> handlerDataEvent;
    EXPECT_CALL(*client, saveDataHandler).WillOnce(SaveArg<0>(&handlerDataEvent));
    EXPECT_CALL(*client, read).Times(1);

    auto timer = std::make_shared<NiceMock<MockTimer>>();
    EXPECT_CALL(*timer, start).Times(1);

    Session<MockServer, MockWriter, MockSocket, MockTimer> session{server, writer, client, timer};

    EXPECT_CALL(writer, push("abcdef")).Times(1);

    auto dataEvent = createPacket("abcdef");
    handlerDataEvent(dataEvent, *client);
}

TEST(Session, repeat) {
    StrictMock<MockServer> server;
    MockWriter writer;

    auto client = std::make_shared<NiceMock<MockSocket>>();

    MockHandle::THandler<uvw::DataEvent> handlerDataEvent;
    EXPECT_CALL(*client, saveDataHandler).WillOnce(SaveArg<0>(&handlerDataEvent));
    EXPECT_CALL(*client, read).Times(1);

    auto timer = std::make_shared<NiceMock<MockTimer>>();
    EXPECT_CALL(*timer, start).Times(1);
    EXPECT_CALL(*timer, again).Times(AtLeast(1));

    Session<MockServer, MockWriter, MockSocket, MockTimer> session{server, writer, client, timer};

    const std::string data1 = "abcdefqwert";
    auto packet1 = createPacket(data1);

    const std::string data2 = "42abcdefqwert";
    auto packet2 = createPacket(data2);

    {
        InSequence _;
        EXPECT_CALL(writer, push(data1)).Times(1);
        EXPECT_CALL(writer, push(data2)).Times(1);
    }

    handlerDataEvent(packet1, *client);
    handlerDataEvent(packet2, *client);
}

TEST(Session, disconnect) {
    StrictMock<MockServer> server;
    NiceMock<MockWriter> writer;

    auto client = std::make_shared<NiceMock<MockSocket>>();

    MockHandle::THandler<uvw::DataEvent> handlerDataEvent;
    EXPECT_CALL(*client, saveDataHandler).WillOnce(SaveArg<0>(&handlerDataEvent));

    MockHandle::THandler<uvw::EndEvent> handlerEndEvent;
    EXPECT_CALL(*client, saveEndHandler).WillOnce(SaveArg<0>(&handlerEndEvent));

    EXPECT_CALL(*client, read).Times(1);

    auto timer = std::make_shared<NiceMock<MockTimer>>();

    Session<MockServer, MockWriter, MockSocket, MockTimer> session{server, writer, client, timer};

    auto createPacket = [](const std::string& data) -> uvw::DataEvent {
        std::uint32_t header = ::htonl(data.size());
        std::size_t packetLength = sizeof(header) + data.size();

        uvw::DataEvent packet{std::make_unique<char[]>(packetLength), packetLength};;

        std::copy_n(reinterpret_cast<const char*>(&header), sizeof(header), packet.data.get());
        std::copy_n(data.data(), data.size(), packet.data.get() + sizeof(header));

        return packet;
    };

    EXPECT_CALL(server, remove(&session)).Times(1);

    auto packet = createPacket("abcdefqwert");
    handlerDataEvent(packet, *client);
    handlerEndEvent(uvw::EndEvent{}, *client);
}
