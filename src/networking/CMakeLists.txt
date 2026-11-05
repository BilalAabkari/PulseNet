file(GLOB NETWORKING_SOURCES "*.cpp" "*.h")
file(GLOB HTTP_SOURCES "http/*.cpp")

set(PLATFORM_SPECIFIC_NETWORKINGS)

#if(WIN32)
#    file(GLOB PLATFORM_SPECIFIC_NETWORKINGS "windows/*.cpp")
#else()
#    file(GLOB PLATFORM_SPECIFIC_NETWORKINGS "unix/*.cpp")
#endif()

if(WIN32)
   
    set(OPENSSL_ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/deps/openssl/windows-x64")
    set(OPENSSL_USE_STATIC_LIBS true)
    find_package(OpenSSL 4.1)

elseif(UNIX)
    find_package(OpenSSL 4.1)
endif()




add_library(networking STATIC ${PLATFORM_SPECIFIC_NETWORKINGS} ${NETWORKING_SOURCES} ${HTTP_SOURCES})

if(WIN32)
    target_link_libraries(networking PRIVATE Ws2_32 Mswsock )
endif()

