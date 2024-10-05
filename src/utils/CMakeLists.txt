file(GLOB UTILS_SOURCES "*.cpp")

add_library(utils STATIC ${UTILS_SOURCES} "Logger.h")
