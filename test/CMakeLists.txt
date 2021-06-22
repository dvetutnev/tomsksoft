add_executable(tests
    sml/test_session.cpp
    mocks.cpp)

target_link_libraries(tests sml::sml uvw::uvw GTest::gmock_main)
add_test(NAME tests COMMAND tests)
