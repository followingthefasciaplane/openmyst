cmake_minimum_required(VERSION 3.20)
project(DreadmystServer VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required packages
find_package(SFML 3 REQUIRED COMPONENTS Network System Window)
find_package(Threads REQUIRED)

# SQLite3 (bundled or system)
find_package(SQLite3)
if(NOT SQLite3_FOUND)
    message(STATUS "SQLite3 not found via find_package, looking for system lib")
    find_library(SQLITE3_LIBRARY sqlite3)
    find_path(SQLITE3_INCLUDE_DIR sqlite3.h)
    if(SQLITE3_LIBRARY AND SQLITE3_INCLUDE_DIR)
        set(SQLite3_FOUND TRUE)
        set(SQLite3_LIBRARIES ${SQLITE3_LIBRARY})
        set(SQLite3_INCLUDE_DIRS ${SQLITE3_INCLUDE_DIR})
    else()
        message(FATAL_ERROR "SQLite3 not found. Install libsqlite3-dev.")
    endif()
endif()

# MySQL client library
find_library(MYSQL_LIBRARY NAMES mysqlclient mariadbclient)
find_path(MYSQL_INCLUDE_DIR mysql/mysql.h PATH_SUFFIXES mysql)
if(NOT MYSQL_LIBRARY)
    find_library(MYSQL_LIBRARY NAMES mysqlclient mariadbclient PATHS /usr/lib/x86_64-linux-gnu)
endif()
if(NOT MYSQL_INCLUDE_DIR)
    find_path(MYSQL_INCLUDE_DIR mysql.h PATH_SUFFIXES mysql mariadb)
endif()
if(NOT MYSQL_LIBRARY OR NOT MYSQL_INCLUDE_DIR)
    message(FATAL_ERROR "MySQL/MariaDB client library not found. Install libmysqlclient-dev or libmariadb-dev.")
endif()

# Collect all source files
file(GLOB_RECURSE SOURCES "src/*.cpp")
file(GLOB_RECURSE HEADERS "src/*.h")

add_executable(dreadmyst-server ${SOURCES} ${HEADERS})

target_include_directories(dreadmyst-server PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${MYSQL_INCLUDE_DIR}
    ${SQLite3_INCLUDE_DIRS}
)

target_link_libraries(dreadmyst-server PRIVATE
    SFML::Network
    SFML::System
    SFML::Window
    ${MYSQL_LIBRARY}
    ${SQLite3_LIBRARIES}
    Threads::Threads
)

# Platform-specific
if(WIN32)
    target_link_libraries(dreadmyst-server PRIVATE ws2_32 winmm)
    target_compile_definitions(dreadmyst-server PRIVATE _WIN32_WINNT=0x0601 NOMINMAX)
endif()

target_compile_definitions(dreadmyst-server PRIVATE
    DREADMYST_VERSION="${PROJECT_VERSION}"
)
