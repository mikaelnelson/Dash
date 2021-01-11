file(GLOB_RECURSE SOURCES libj1939/src/*.c)
idf_component_register(SRCS ${SOURCES}
        INCLUDE_DIRS . libj1939/src libj1939/include)

target_compile_definitions(${COMPONENT_LIB} INTERFACE)
