#include "session.h"
#include "mocks.h"


using ::testing::SaveArg;
using ::testing::InSequence;
using ::testing::StrictMock;
using ::testing::NiceMock;


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

    auto client = std::make_shared<StrictMock<MockSocket>>();

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
    handlerErrorEvent(uvw::ErrorEvent{static_cast<std::underlying_type_t<uv_errno_t>>(UV_EFAULT)}, *timer);
}
