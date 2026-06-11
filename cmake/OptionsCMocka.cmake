# Disable things we don't need from CMocka
set(WITH_EXAMPLES OFF CACHE BOOL "Build examples" FORCE)
set(UNIT_TESTING OFF CACHE BOOL "Build CMocka's own tests" FORCE)
set(WITH_CMOCKERY_SUPPORT OFF CACHE BOOL "" FORCE)

# Force static library (easier for most projects)
set(WITH_STATIC_LIB ON CACHE BOOL "Build static library" FORCE)