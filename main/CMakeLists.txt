
# Libraries
list( APPEND LIBS lvgl )
list( APPEND LIBS lvgl_tft )
list( APPEND LIBS lvgl_drivers )
list( APPEND LIBS minmea)
list( APPEND LIBS pubsub-c)

# Source Files
list( APPEND SRC_FILES main.c )
list( APPEND SRC_FILES stepper_gauge.c )
list( APPEND SRC_FILES speedometer_gauge.c )
list( APPEND SRC_FILES can_j1939.c )
list( APPEND SRC_FILES gps.c )
list( APPEND SRC_FILES display.c )

# Include Directories


idf_component_register(
        SRCS            ${SRC_FILES}
        INCLUDE_DIRS    ${INC_DIRS}
        REQUIRES        ${LIBS}
)

# Build static library, do not build test executables
option(BUILD_SHARED_LIBS OFF)
option(BUILD_TESTING OFF)

# Import Targets
add_subdirectory(lib/libj1939)

# Include Directories
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/lib/libj1939/include)

# Link Target To Main Component
target_link_libraries(${COMPONENT_LIB} PUBLIC j1939)
