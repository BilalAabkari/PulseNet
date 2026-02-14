include(FetchContent)

FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
if(WIN32)
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
endif()

FetchContent_MakeAvailable(googletest)

enable_testing()

set(TEST_SOURCES
    networking/HttpAssemblerTests.cpp
    networking/UtilsTests.cpp
)

add_executable(PulseNetTests ${TEST_SOURCES})

target_link_libraries(PulseNetTests PRIVATE
    pulsenet_lib GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(PulseNetTests)