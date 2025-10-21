file(GLOB NETWORKING_SOURCES "*.cpp")
file(GLOB HTTP_SOURCES "http/*.cpp")

set(PLATFORM_SPECIFIC_NETWORKINGS)

if(WIN32)
    file(GLOB PLATFORM_SPECIFIC_NETWORKINGS "windows/*.cpp")
else()
    file(GLOB PLATFORM_SPECIFIC_NETWORKINGS "unix/*.cpp")
endif()


add_library(networking STATIC ${PLATFORM_SPECIFIC_NETWORKINGS} ${NETWORKING_SOURCES} ${HTTP_SOURCES})

if(WIN32)
    target_link_libraries(networking PRIVATE Ws2_32 Mswsock)
endif()
