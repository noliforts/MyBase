cmake_minimum_required(VERSION 3.16)
project(MyBase VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(src/db_core)
add_subdirectory(src/db_server)
add_subdirectory(src/db_client_lib)
add_subdirectory(src/db_client)
add_subdirectory(tests)