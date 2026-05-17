cmake_minimum_required(VERSION 3.16)
project(MyBase VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(include)   # добавляем

# 1. Модуль ядра СУБД
file(GLOB_RECURSE CORE_SRC "src/db_core/*.cpp")
add_library(db_core STATIC ${CORE_SRC})

# 2. Модуль клиентской библиотеки
file(GLOB_RECURSE CLIENT_LIB_SRC "src/db_client_lib/*.cpp")
add_library(db_client_lib STATIC ${CLIENT_LIB_SRC})

# 3. Модуль сервера СУБД
file(GLOB_RECURSE SERVER_SRC "src/db_server/*.cpp")
add_executable(db_server ${SERVER_SRC})
target_link_libraries(db_server db_core)

# 4. Модуль консольного клиента
file(GLOB_RECURSE CLIENT_SRC "src/db_client/*.cpp")
add_executable(db_client ${CLIENT_SRC})
target_link_libraries(db_client db_client_lib)

# Тесты (включаются при сборке)
#enable_testing()
#file(GLOB TEST_FILES "tests/*.cpp")
#foreach(TEST_FILE ${TEST_FILES})
#get_filename_component(TEST_NAME ${TEST_FILE} NAME_WE)
#add_executable(${TEST_NAME} ${TEST_FILE})
#target_link_libraries(${TEST_NAME} db_core db_client_lib)
#add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})
endforeach()
