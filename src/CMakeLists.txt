#The executable 
file(GLOB MAIN_SOURCES "*.cpp")

add_subdirectory(utils)
add_subdirectory(networking)

add_executable(PulseNet ${MAIN_SOURCES})

target_link_libraries(PulseNet PRIVATE
    utils
    networking
)

target_compile_definitions(networking PRIVATE $<$<CONFIG:Debug>:DEBUG>)

#The library for tests
add_library(pulsenet_lib STATIC Commands.cpp)
target_include_directories(pulsenet_lib
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(pulsenet_lib PUBLIC networking utils)
