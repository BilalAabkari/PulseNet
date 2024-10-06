cmake_minimum_required (VERSION 3.8)
project ("PulseNet")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(utils)
add_subdirectory(database)
add_subdirectory(networking)

add_executable (PulseNet "PulseNet.cpp" "PulseNet.h" )



target_link_libraries(PulseNet PRIVATE
    database
    utils
    networking
    $<$<PLATFORM_ID:Windows>:Ws2_32>
)



