#pragma once

#include <gmock/gmock.h>
#include <uvw.hpp>

#include <chrono>
#include <functional>
#include <type_traits>


class SessionBase;

struct MockServer
{
    MOCK_METHOD(void, remove, (SessionBase*), ());
};

struct MockWriter
{
    MOCK_METHOD(void, push, (const std::string&), ());
};


struct MockHandle
{
    template <typename E>
    using THandler = std::function<void(const E&, MockHandle&)>;

    virtual ~MockHandle() = default;
};

struct MockTimer : MockHandle
{
    using Duration = std::chrono::steady_clock::duration;

    MOCK_METHOD(void, start, (Duration, Duration), ());
    MOCK_METHOD(void, again, (), ());
    MOCK_METHOD(void, stop, (), ());
    MOCK_METHOD(void, close, (), ());

    MOCK_METHOD(void, saveHandler, (THandler<uvw::TimerEvent>), ());

    template <typename E>
    void on(THandler<E> h) { saveHandler(h); }
};

struct MockSocket : MockHandle
{
    MOCK_METHOD(void, read, (), ());
    MOCK_METHOD(void, stop, (), ());
    MOCK_METHOD(void, close, (), ());

    MOCK_METHOD(void, saveDataHandler, (THandler<uvw::DataEvent>), ());
    MOCK_METHOD(void, saveErrorHandler, (THandler<uvw::ErrorEvent>), ());

    template <typename E>
    void on(THandler<E> h) {
        if constexpr (std::is_same_v<E, uvw::DataEvent>) {
            saveDataHandler(h);
        }
        else if constexpr (std::is_same_v<E, uvw::ErrorEvent>) {
            saveErrorHandler(h);
        }
    }
};

struct MockFile : MockHandle
{
    MOCK_METHOD(void, write, (std::unique_ptr<char[]>, unsigned int, std::int64_t));

    MOCK_METHOD(void, saveWriteHandler, (THandler<uvw::FsEvent<uvw::FileReq::Type::WRITE>>), ());
    MOCK_METHOD(void, saveErrorHandler, (THandler<uvw::ErrorEvent>), ());

    template <typename E>
    void on(THandler<E> h) {
        if constexpr (std::is_same_v<E, uvw::FsEvent<uvw::FileReq::Type::WRITE>>) {
            saveWriteHandler(h);
        }
        else if constexpr (std::is_same_v<E, uvw::ErrorEvent>) {
            saveErrorHandler(h);
        }
    }
};
