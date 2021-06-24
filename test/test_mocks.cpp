#include "mocks.h"


using ::testing::SaveArg;
using ::testing::NotNull;
using ::testing::Unused;
using ::testing::_;


namespace {


struct MockHandler
{
    MOCK_METHOD(void, onDataEvent, (const uvw::DataEvent&), ());
    MOCK_METHOD(void, onErrorEvent, (const uvw::ErrorEvent&), ());
    MOCK_METHOD(void, onTimerEvent, (const uvw::TimerEvent&), ());
    MOCK_METHOD(void, onEndEvent, (const uvw::EndEvent&), ());
};


} // Anonymous namespace


TEST(MockSocket, saveHandler) {
    MockHandler handler;

    EXPECT_CALL(handler, onDataEvent).Times(1);
    EXPECT_CALL(handler, onErrorEvent).Times(1);
    EXPECT_CALL(handler, onEndEvent).Times(1);

    MockHandle::THandler<uvw::DataEvent> handlerDataEvent = [&handler](const uvw::DataEvent& e, auto&) { handler.onDataEvent(e); };
    MockHandle::THandler<uvw::ErrorEvent> handlerErrorEvent = [&handler](const uvw::ErrorEvent& e, auto&) { handler.onErrorEvent(e); };
    MockHandle::THandler<uvw::EndEvent> handlerEndEvent = [&handler](const uvw::EndEvent& e, auto&) { handler.onEndEvent(e); };

    MockSocket mock;

    MockHandle::THandler<uvw::DataEvent> savedHandlerDataEvent = nullptr;
    MockHandle::THandler<uvw::ErrorEvent> savedHandlerErrorEvent = nullptr;
    MockHandle::THandler<uvw::EndEvent> savedHandlerEndEvent = nullptr;

    EXPECT_CALL(mock, saveDataHandler).WillOnce(SaveArg<0>(&savedHandlerDataEvent));
    EXPECT_CALL(mock, saveErrorHandler).WillOnce(SaveArg<0>(&savedHandlerErrorEvent));
    EXPECT_CALL(mock, saveEndHandler).WillOnce(SaveArg<0>(&savedHandlerEndEvent));

    mock.on<uvw::DataEvent>(handlerDataEvent);
    mock.on<uvw::ErrorEvent>(handlerErrorEvent);
    mock.on<uvw::EndEvent>(handlerEndEvent);

    savedHandlerDataEvent(uvw::DataEvent{nullptr, 0}, mock);
    savedHandlerErrorEvent(uvw::ErrorEvent{static_cast<std::underlying_type_t<uv_errno_t>>(UV_EFAULT)}, mock);
    savedHandlerEndEvent(uvw::EndEvent{}, mock);
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

TEST(MockFile, _) {
    MockFile mock;

    std::unique_ptr<char[]> saved = nullptr;
    EXPECT_CALL(mock, write(NotNull(), 42, 43)).WillOnce( [&saved] (std::unique_ptr<char[]> p, Unused, Unused) { saved = std::move(p); } );

    auto p = std::make_unique<char[]>(37);
    auto pRaw = p.get();
    mock.write(std::move(p), 42, 43);

    ASSERT_EQ(saved.get(), pRaw);
}
