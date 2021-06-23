add_executable(tests
    sml/test_session.cpp
    sml/test_defer.cpp
    test_mocks.cpp
    test_session.cpp
    test_writer.cpp)

target_link_libraries(tests sml::sml uvw::uvw GTest::gmock_main)
add_test(NAME tests COMMAND tests)
