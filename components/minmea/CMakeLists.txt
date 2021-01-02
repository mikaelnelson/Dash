idf_component_register(SRCS minmea/minmea.c
        INCLUDE_DIRS . minmea)

target_compile_definitions(${COMPONENT_LIB} INTERFACE MINMEA_INCLUDE_COMPAT=1)
