#include "mocks.h"


using ::testing::SaveArg;


namespace {


struct MockHandler
{
    MOCK_METHOD(void, onDataEvent, (const uvw::DataEvent&), ());
    MOCK_METHOD(void, onErrorEvent, (const uvw::ErrorEvent&), ());
};


} //Anonymous namespace


TEST(MockSocket, save_handler) {
    MockHandler handler;

    EXPECT_CALL(handler, onDataEvent).Times(1);
    EXPECT_CALL(handler, onErrorEvent).Times(1);

    MockHandle::THandler<uvw::DataEvent> handlerDataEvent = [&handler](const uvw::DataEvent& e, auto&) { handler.onDataEvent(e); };
    MockHandle::THandler<uvw::ErrorEvent> handlerErrorEvent = [&handler](const uvw::ErrorEvent& e, auto&) { handler.onErrorEvent(e); };

    MockSocket mock;

    MockHandle::THandler<uvw::DataEvent> savedHandlerDataEvent = nullptr;
    MockHandle::THandler<uvw::ErrorEvent> savedHandlerErrorEvent = nullptr;

    EXPECT_CALL(mock, saveReadHandler).WillOnce(SaveArg<0>(&savedHandlerDataEvent));
    EXPECT_CALL(mock, saveErrorHandler).WillOnce(SaveArg<0>(&savedHandlerErrorEvent));

    mock.on<uvw::DataEvent>(handlerDataEvent);
    mock.on<uvw::ErrorEvent>(handlerErrorEvent);

    savedHandlerDataEvent(uvw::DataEvent{nullptr, 0}, mock);
    savedHandlerErrorEvent(uvw::ErrorEvent{static_cast<std::underlying_type_t<uv_errno_t>>(UV_EFAULT)}, mock);
}
