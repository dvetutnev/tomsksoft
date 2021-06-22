add_executable(tests
    sml/test_session.cpp)

target_link_libraries(tests sml::sml GTest::gmock_main)
add_test(NAME tests COMMAND tests)
