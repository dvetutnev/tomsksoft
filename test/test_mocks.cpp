#include "mocks.h"


using ::testing::SaveArg;


namespace {


struct MockHandler
{
    MOCK_METHOD(void, onDataEvent, (const uvw::DataEvent&), ());
    MOCK_METHOD(void, onErrorEvent, (const uvw::ErrorEvent&), ());
    MOCK_METHOD(void, onTimerEvent, (const uvw::TimerEvent&), ());
};


} // Anonymous namespace


TEST(MockSocket, saveHandler) {
    MockHandler handler;

    EXPECT_CALL(handler, onDataEvent).Times(1);
    EXPECT_CALL(handler, onErrorEvent).Times(1);

    MockHandle::THandler<uvw::DataEvent> handlerDataEvent = [&handler](const uvw::DataEvent& e, auto&) { handler.onDataEvent(e); };
    MockHandle::THandler<uvw::ErrorEvent> handlerErrorEvent = [&handler](const uvw::ErrorEvent& e, auto&) { handler.onErrorEvent(e); };

    MockSocket mock;

    MockHandle::THandler<uvw::DataEvent> savedHandlerDataEvent = nullptr;
    MockHandle::THandler<uvw::ErrorEvent> savedHandlerErrorEvent = nullptr;

    EXPECT_CALL(mock, saveDataHandler).WillOnce(SaveArg<0>(&savedHandlerDataEvent));
    EXPECT_CALL(mock, saveErrorHandler).WillOnce(SaveArg<0>(&savedHandlerErrorEvent));

    mock.on<uvw::DataEvent>(handlerDataEvent);
    mock.on<uvw::ErrorEvent>(handlerErrorEvent);

    savedHandlerDataEvent(uvw::DataEvent{nullptr, 0}, mock);
    savedHandlerErrorEvent(uvw::ErrorEvent{static_cast<std::underlying_type_t<uv_errno_t>>(UV_EFAULT)}, mock);
}

TEST(MockTimer, saveHandler) {
    MockHandler handler;

    EXPECT_CALL(handler, onTimerEvent).Times(1);

    MockHandle::THandler<uvw::TimerEvent> handlerTimerEvent = [&handler](const uvw::TimerEvent& e, auto&) { handler.onTimerEvent(e); };

    MockTimer mock;

    MockHandle::THandler<uvw::TimerEvent> savedHandlerTimerEvent = nullptr;

    EXPECT_CALL(mock, saveHandler).WillOnce(SaveArg<0>(&savedHandlerTimerEvent));

    mock.on<uvw::TimerEvent>(handlerTimerEvent);

    savedHandlerTimerEvent(uvw::TimerEvent{}, mock);
}

TEST(MockWriter, push) {
    MockWriter mock;
    const std::string data = "0123456789";
    EXPECT_CALL(mock, push_rvr(data)).Times(1);
    mock.push(std::string{data});
}
