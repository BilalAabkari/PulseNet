cmake_minimum_required (VERSION 3.8)

project ("PulseNet")

add_executable (PulseNet "src/PulseNet.cpp" "src/PulseNet.h" "src/utils/ConfigParser.h" "src/utils/ConfigParser.cpp" "src/database/DatabaseConnection.h" "src/database/DatabaseConnection.cpp")

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET PulseNet PROPERTY CXX_STANDARD 20)
endif()

