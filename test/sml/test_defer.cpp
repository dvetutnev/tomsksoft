#include <gmock/gmock.h>
#include <boost/sml.hpp>

#include <deque>


using ::testing::InSequence;
using ::testing::StrictMock;


namespace {


struct Mock
{
    MOCK_METHOD(void, write, (int), ());
    MOCK_METHOD(void, close, (), ());
};


struct DataEvent { int value; };
struct WrittenEvent {};
struct ShutdownEvent {};

struct DefFSM
{
    Mock& mock;
    explicit DefFSM(Mock& m) : mock{m} {}

    auto operator()() {
        auto write = [this] (const DataEvent& e) { mock.write(e.value); };
        auto close = [this] (const ShutdownEvent&) { mock.close(); };

        using namespace boost::sml;

        return make_transition_table(
                    *"Wait"_s + event<DataEvent> / write = "Write"_s,
                     "Wait"_s + event<ShutdownEvent> / close = X,
                     "Write"_s + event<DataEvent> / defer,
                     "Write"_s + event<ShutdownEvent> / defer,
                     "Write"_s + event<WrittenEvent> = "Wait"_s
        );
    }
};


} // Anonymous namespace


TEST(Boost_SML, defer) {
    StrictMock<Mock> mock;
    {
        InSequence _;
        EXPECT_CALL(mock, write(1));
        EXPECT_CALL(mock, write(2));
        EXPECT_CALL(mock, close);
    }

    DefFSM defFSM{mock};
    boost::sml::sm<DefFSM, boost::sml::defer_queue<std::deque>> fsm{defFSM};

    fsm.process_event(DataEvent{1});
    fsm.process_event(DataEvent{2});
    fsm.process_event(ShutdownEvent{});
    fsm.process_event(WrittenEvent{});
    fsm.process_event(WrittenEvent{});
}
