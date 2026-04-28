cmake_minimum_required(VERSION 3.14)

project(fitness-tracker-userver CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/third_party/userver/CMakeLists.txt")
    message(FATAL_ERROR
        "Vendored userver was not found at third_party/userver. "
        "Run: git submodule update --init --recursive"
    )
endif()

add_subdirectory(third_party/userver)

if(COMMAND userver_setup_environment)
    userver_setup_environment()
endif()

add_executable(${PROJECT_NAME}
    src/api_utils.cpp
    src/fitness_storage.cpp
    src/handlers.cpp
    src/main.cpp
)
target_link_libraries(${PROJECT_NAME} PRIVATE userver::core)
