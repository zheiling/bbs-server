add_library(db STATIC)

if (USE_POSTGRESQL)
    target_link_libraries(db
        PRIVATE
            PostgreSQL::PostgreSQL
    )
else()
    target_link_libraries(db
        PRIVATE
            sqlite
    )
endif()

target_sources(db
    PRIVATE
        "db.c"
    PUBLIC
        FILE_SET HEADERS
        FILES
           "db.h"
)