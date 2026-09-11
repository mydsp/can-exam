if(CAN_DUAL_NODE)
    set(CAN_APP_SOURCES "dual_node.c")
else()
    set(CAN_APP_SOURCES "selftest.c")
endif()

idf_component_register(SRCS ${CAN_APP_SOURCES}
                       INCLUDE_DIRS "."
                       REQUIRES esp_driver_twai)

if(CAN_DUAL_NODE)
    if(CAN_ROLE STREQUAL "master")
        target_compile_definitions(${COMPONENT_LIB} PRIVATE CAN_ROLE_MASTER=1)
    elseif(CAN_ROLE STREQUAL "slave")
        target_compile_definitions(${COMPONENT_LIB} PRIVATE CAN_ROLE_SLAVE=1)
    else()
        message(FATAL_ERROR "CAN_ROLE must be master or slave when CAN_DUAL_NODE=1")
    endif()
endif()
