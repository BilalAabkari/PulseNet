cmake_minimum_required (VERSION 3.8)
project ("PulseNet")

add_subdirectory(utils)
add_subdirectory(database)
add_subdirectory(networking)

add_executable (PulseNet "PulseNet.cpp" "PulseNet.h" )
if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET PulseNet PROPERTY CXX_STANDARD 20)
endif()


target_link_libraries(PulseNet PRIVATE
    database
    utils
    networking
    $<$<PLATFORM_ID:Windows>:Ws2_32>
)



