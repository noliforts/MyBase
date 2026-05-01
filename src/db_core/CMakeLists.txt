add_library(db_core STATIC
    lexer.cpp
    parser.cpp
    commands.cpp
    condition.cpp
    table_schema.cpp
    table.cpp
    database.cpp
    database_manager.cpp
    storage_engine.cpp
)

target_include_directories(db_core PUBLIC
    ${CMAKE_SOURCE_DIR}/include/db_core
)