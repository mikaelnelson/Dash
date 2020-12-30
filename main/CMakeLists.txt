

idf_component_register(
        SRCS
        "main.c"
        "stepper_gauge.c"
        "can_j1939.c"

        INCLUDE_DIRS
        ""
)

# Build static library, do not build test executables
option(BUILD_SHARED_LIBS OFF)
option(BUILD_TESTING OFF)

# Import tinyxml2 targets
add_subdirectory(lib/libj1939)

# Include J1939 Directories
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/lib/libj1939/include)

# Link tinyxml2 to main component
target_link_libraries(${COMPONENT_LIB} PUBLIC j1939)