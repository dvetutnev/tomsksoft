#pragma once

#include <boost/sml.hpp>
#include <uvw.hpp>

#include <deque>
#include <algorithm>


template <typename File>
class Writer
{
public:
    Writer(std::shared_ptr<File>);

    void push(const std::string&);
    void shutdown();

private:
    std::shared_ptr<File> file;

    void writeToFile(const std::string&);
    void closeFile();

    std::int64_t offset = 0;

    struct DefFSM
    {
        explicit DefFSM(Writer& w) : writer{w} {}

        struct DataEvent { std::string data; };
        struct WrittenEvent {};
        struct ShutdownEvent {};

        auto operator()() const {

            auto write = [this] (const DataEvent& e) { writer.writeToFile(e.data); };
            auto close = [this] (const ShutdownEvent&) { writer.closeFile(); };

            using namespace boost::sml;

            return make_transition_table(
                *"Wait"_s   + event<DataEvent>      / write = "Write"_s,
                 "Wait"_s   + event<ShutdownEvent>  / close = X,
                 "Write"_s  + event<DataEvent>      / defer,
                 "Write"_s  + event<ShutdownEvent>  / defer,
                 "Write"_s  + event<WrittenEvent>           = "Wait"_s
            );
        }

        Writer& writer;
    };

    DefFSM defFsm;
    boost::sml::sm<DefFSM, boost::sml::defer_queue<std::deque>> fsm;
};


template <typename File>
Writer<File>::Writer(std::shared_ptr<File> f)
    :
      file{std::move(f)},

      defFsm{*this},
      fsm{defFsm}
{
    using WriteEvent = uvw::FsEvent<uvw::FileReq::Type::WRITE>;
    auto writeHandler = [this] (const WriteEvent& e, auto&) {
        offset += e.size;
        fsm.process_event(typename DefFSM::WrittenEvent{});
    };
    file->template on<WriteEvent>(writeHandler);
}


template <typename File>
void Writer<File>::push(const std::string& d) {
    typename DefFSM::DataEvent e{d};
    fsm.process_event(e);
}

template <typename File>
void Writer<File>::shutdown() {
    typename DefFSM::ShutdownEvent e;
    fsm.process_event(e);
}


template <typename File>
void Writer<File>::writeToFile(const std::string& data) {
    auto d = std::make_unique<char[]>(data.size());
    std::copy_n(std::begin(data), data.size(), d.get());
    file->write(std::move(d), data.size(), offset);
}

template <typename File>
void Writer<File>::closeFile() {
    file->close();
}
