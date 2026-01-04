cmake_minimum_required (VERSION 3.8)
project ("PulseNet Tests")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

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

target_compile_definitions(networking PRIVATE $<$<CONFIG:Debug>:DEBUG>)

set(TEST_SOURCES networking/HttpAssemblerTests.cpp)

add_executable(PulseNetTests TEST_SOURCES)

target_link_libraries(PulseNetTests PRIVATE
    networking GTest::gtest_main
)