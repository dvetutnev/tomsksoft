add_executable(tests
    sml/test_session.cpp
    sml/test_defer.cpp
    test_mocks.cpp
    test_session.cpp
    test_writer.cpp
    test_uvw.cpp
    test_functional.cpp
    test_parse_config.cpp)

target_link_libraries(tests
    sml::sml
    uvw::uvw
    libuv::libuv
    nlohmann_json::nlohmann_json
    GTest::gmock_main)

add_test(NAME tests COMMAND tests)
