cmake_minimum_required(VERSION 3.24)

project(MemoryGame LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(MEMORY_GAME_FETCH_SFML "Fetch SFML 3.x with CMake FetchContent" OFF)

if(MEMORY_GAME_FETCH_SFML)
    include(FetchContent)

    set(SFML_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(SFML_BUILD_TEST_SUITE OFF CACHE BOOL "" FORCE)
    set(SFML_BUILD_DOC OFF CACHE BOOL "" FORCE)

    FetchContent_Declare(
        SFML
        GIT_REPOSITORY https://github.com/SFML/SFML.git
        GIT_TAG 3.0.2
        GIT_SHALLOW ON
        EXCLUDE_FROM_ALL
        SYSTEM
    )
    FetchContent_MakeAvailable(SFML)
else()
    find_package(SFML 3 COMPONENTS Graphics Window System CONFIG REQUIRED)
endif()

add_executable(memory_game
    src/main.cpp
)

target_link_libraries(memory_game
    PRIVATE
        SFML::Graphics
        SFML::Window
        SFML::System
)

if(WIN32 AND TARGET SFML::Main)
    target_link_libraries(memory_game PRIVATE SFML::Main)
endif()

set_target_properties(memory_game PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

add_custom_command(
    TARGET memory_game
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_SOURCE_DIR}/assets"
            "$<TARGET_FILE_DIR:memory_game>/assets"
)
