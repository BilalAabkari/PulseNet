file(GLOB DATABASE_SOURCES "*.cpp")




find_package(PostgreSQL REQUIRED)

add_library(database STATIC ${DATABASE_SOURCES} )

target_link_libraries(database PUBLIC PostgreSQL::PostgreSQL)




