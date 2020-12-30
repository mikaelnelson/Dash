

idf_component_register(
        SRCS
        "main.c"
        "stepper_gauge.c"
        "can_j1939.c"

        INCLUDE_DIRS
        ""
)

# Include Directories
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/external/install/libj1939/include/)

## Add Dependencies
add_dependencies(__idf_main libj1939)
#
## Link Libraries
target_link_libraries(__idf_main ${CMAKE_CURRENT_SOURCE_DIR}/external/install/libj1939/lib/libj1939.a)