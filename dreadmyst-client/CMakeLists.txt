cmake_minimum_required(VERSION 3.20)
project(DreadmystClient VERSION 1.0.1189 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Dependencies
find_package(SFML 3 REQUIRED COMPONENTS Graphics Window System Network Audio)
find_package(SQLite3 REQUIRED)
find_package(ZLIB REQUIRED)
find_package(libzip REQUIRED)
find_package(Threads REQUIRED)

# Collect sources
file(GLOB_RECURSE CLIENT_SOURCES src/*.cpp)

# Shared library sources (StlBuffer, Config, etc.)
set(SHARED_DIR ${CMAKE_SOURCE_DIR}/../Shared)
set(SHARED_SOURCES
    ${CMAKE_SOURCE_DIR}/src/shared/StlBuffer.cpp
    ${CMAKE_SOURCE_DIR}/src/shared/Config.cpp
    ${CMAKE_SOURCE_DIR}/src/shared/SfSocket.cpp
)

add_executable(${PROJECT_NAME} ${CLIENT_SOURCES} ${SHARED_SOURCES})

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${SHARED_DIR}
    ${SQLite3_INCLUDE_DIRS}
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    SFML::Graphics
    SFML::Window
    SFML::System
    SFML::Network
    SFML::Audio
    SQLite::SQLite3
    ZLIB::ZLIB
    libzip::zip
    Threads::Threads
)

target_compile_definitions(${PROJECT_NAME} PRIVATE
    DMYST_CLIENT
)
