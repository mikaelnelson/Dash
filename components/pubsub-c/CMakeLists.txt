set(COMPONENT_SRCS "pubsub-c/src/pubsub.c")
set(COMPONENT_ADD_INCLUDEDIRS "pubsub-c/src")

register_component()

set_source_files_properties(pubsub-c/src/pubsub.c
    PROPERTIES COMPILE_FLAGS -DPS_FREE_RTOS
)
