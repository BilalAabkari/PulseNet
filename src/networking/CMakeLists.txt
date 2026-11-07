file(GLOB NETWORKING_SOURCES "*.cpp" "*.h")
file(GLOB HTTP_SOURCES "http/*.cpp")
file(GLOB TLS_SOURCES "tls/*.cpp" "tls/*.h")

set(PLATFORM_SPECIFIC_NETWORKINGS)

#if(WIN32)
#    file(GLOB PLATFORM_SPECIFIC_NETWORKINGS "windows/*.cpp")
#else()
#    file(GLOB PLATFORM_SPECIFIC_NETWORKINGS "unix/*.cpp")
#endif()

if(WIN32)
   
    set(OPENSSL_ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/deps/openssl/windows-x64")
    set(OPENSSL_USE_STATIC_LIBS true)
    find_package(OpenSSL 4.1 REQUIRED)

  
elseif(UNIX)
    find_package(OpenSSL 4.1 REQUIRED)
endif()




add_library(networking STATIC  ${NETWORKING_SOURCES} ${HTTP_SOURCES} ${TLS_SOURCES} )

if(WIN32)
    target_link_libraries(networking PUBLIC Ws2_32 Mswsock OpenSSL::SSL OpenSSL::Crypto)
endif()

