cmake_minimum_required (VERSION 3.8)
project ("PulseNet")

file(GLOB MAIN_SOURCES "*.cpp")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(utils)
add_subdirectory(networking)

add_executable (PulseNet ${MAIN_SOURCES}  )

target_link_libraries(PulseNet PRIVATE
    utils
    networking
)



