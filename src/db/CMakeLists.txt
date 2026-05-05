add_library(db STATIC)

if (USE_POSTGRESQL)
    target_compile_definitions(db PRIVATE USE_POSTGRESQL)
    target_link_libraries(db
        PRIVATE
            PostgreSQL::PostgreSQL
    )
else()
    target_link_libraries(db
        PRIVATE
            SQLite3::SQLite3
    )
endif()

target_sources(db
    PRIVATE
        "db.c"
        "db_common.c"
    PUBLIC
        FILE_SET HEADERS
        FILES
           "db.h"
)