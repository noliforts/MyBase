

cmake_minimum_required(VERSION 3.14)
project(MyBase CXX)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(POLICY CMP0135)
  cmake_policy(SET CMP0135 NEW)
endif()

include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/heads/main.zip
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

FetchContent_Declare(
  replxx
  URL https://github.com/AmokHuginnsson/replxx/archive/refs/tags/release-0.0.4.zip
)
set(REPLXX_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(REPLXX_BUILD_PACKAGE  OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(replxx)

include_directories(include)

file(GLOB_RECURSE CORE_SOURCES src/db_core/*.cpp)
add_library(db_core STATIC ${CORE_SOURCES})

file(GLOB_RECURSE SERVER_SOURCES src/db_server/*.cpp)
add_executable(db_server ${SERVER_SOURCES})
target_link_libraries(db_server PRIVATE db_core)



file(GLOB_RECURSE CLIENT_LIB_SOURCES src/db_client_lib/*.cpp)
add_library(db_client_lib STATIC ${CLIENT_LIB_SOURCES})
# Связываем её с db_core, чтобы клиент видел структуры ответов (QueryResult)
target_link_libraries(db_client_lib PRIVATE db_core)

# Б) Собираем сам консольный клиент (db_client)
file(GLOB_RECURSE CLIENT_CLI_SOURCES src/db_client/*.cpp)
add_executable(db_client ${CLIENT_CLI_SOURCES})
# Линкуем клиент исключительно с его библиотекой по ТЗ!
target_link_libraries(db_client PRIVATE db_client_lib replxx)
target_include_directories(db_client PRIVATE ${replxx_SOURCE_DIR}/include)



enable_testing()

file(GLOB TEST_SOURCES tests/*.cpp)

add_executable(db_tests ${TEST_SOURCES})
target_include_directories(db_tests PRIVATE ${googletest_SOURCE_DIR}/googletest/include)
target_link_libraries(db_tests PRIVATE db_core gtest gtest_main)
add_test(NAME AllTests COMMAND db_tests)
