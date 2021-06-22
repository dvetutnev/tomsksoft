#include <boost/sml.hpp>
#include <gmock/gmock.h>


namespace sml = boost::sml;

using ::testing::InSequence;
using ::testing::Return;


namespace {


struct Mock
{
    MOCK_METHOD(void, init, (), ());
    MOCK_METHOD(void, push_to_header_buffer, (char), ());
    MOCK_METHOD(bool, is_header_complete, (), ());
    MOCK_METHOD(void, repeat, (), ());
};


struct ByteEvent { char byte; };
struct HaltEvent {};


struct Session
{
    Mock& mock;

    explicit Session(Mock& m) : mock{m} {}

    auto operator()() const {
        using namespace sml;

        Mock& m = mock;

        auto init = [&m] () { m.init(); };
        auto push_to_header_buffer = [&m] (const ByteEvent& event) { m.push_to_header_buffer(event.byte); };
        auto is_header_complete = [&m] () { return m.is_header_complete(); };
        auto repeat = [&m] () { m.repeat(); };


        return make_transition_table(
            *"Init"_s / init = "Receive header"_s
,
            "Receive header"_s + event<ByteEvent> / push_to_header_buffer = "Is header complete"_s
,
            "Is header complete"_s [ is_header_complete ] = "Receive data"_s
,
            "Is header complete"_s  = "Receive header"_s
,
            "Receive data"_s / repeat = "Receive header"_s
        );
    }
};


} // Anonymous namespace


TEST(sml, session) {
    Mock mock;

    EXPECT_CALL(mock, init).Times(1);

    {
        InSequence _;
        EXPECT_CALL(mock, push_to_header_buffer(42));
        EXPECT_CALL(mock, push_to_header_buffer(117));
    }

    EXPECT_CALL(mock, is_header_complete()).WillOnce(Return(false)).WillOnce(Return(true));

    EXPECT_CALL(mock, repeat);


    Session session{mock};
    sml::sm<Session> sm{session};

    sm.process_event(ByteEvent{42});
    sm.process_event(ByteEvent{117});
}
