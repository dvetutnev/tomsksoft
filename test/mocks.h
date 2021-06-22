#pragma once

#include <gmock/gmock.h>
#include <uvw.hpp>

#include <chrono>
#include <functional>
#include <type_traits>


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

    MOCK_METHOD(void, saveHandler, (THandler<uvw::TimerEvent>), ());

    template <typename E>
    void on(THandler<E> h) { saveHandler(h); }
};

struct MockSocket : MockHandle
{
    MOCK_METHOD(void, saveReadHandler, (THandler<uvw::DataEvent>), ());
    MOCK_METHOD(void, saveErrorHandler, (THandler<uvw::ErrorEvent>), ());

    template <typename E>
    void on(THandler<E> h) {
        if constexpr (std::is_same_v<E, uvw::DataEvent>) {
            saveReadHandler(h);
        }
        else if constexpr (std::is_same_v<E, uvw::ErrorEvent>) {
            saveErrorHandler(h);
        }
    }
};
