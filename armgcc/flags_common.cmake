IF(NOT DEFINED FPU)  
    SET(FPU "-mfloat-abi=hard -mfpu=fpv5-d16")  
ENDIF()

IF(NOT DEFINED SPECS)  
    SET(SPECS "--specs=nano.specs --specs=nosys.specs")  
ENDIF()

SET(DEBUG_CONSOLE_CONFIG "-DSDK_DEBUGCONSOLE=2")  

IF(NOT DEFINED CMAKE_ASM_FLAGS_BASE)
SET(CMAKE_ASM_FLAGS_BASE " \
    -D__STARTUP_CLEAR_BSS \
    -D__STARTUP_INITIALIZE_NONCACHEDATA \
    -mcpu=cortex-m7 \
    -mthumb \
    ${FPU} \
")
ENDIF()

IF(NOT DEFINED CMAKE_C_FLAGS_BASE)  
SET(CMAKE_C_FLAGS_BASE " \
    -DCPU_MIMXRT1176DVMAA_cm7 \
    -DMCUXPRESSO_SDK \
    -DSERIAL_PORT_TYPE_UART=1 \
    -D_POSIX_SOURCE \
    -DLWIP_ENET_FLEXIBLE_CONFIGURATION \
    -DFSL_SDK_ENABLE_DRIVER_CACHE_CONTROL=1 \
    -DFSL_FEATURE_PHYKSZ8081_USE_RMII50M_MODE \
    -DUSE_RTOS=1 \
    -DPRINTF_ADVANCED_ENABLE=1 \
    -DETH_LINK_POLLING_INTERVAL_MS=0 \
    -DFSL_SDK_DRIVER_QUICK_ACCESS_ENABLE=1 \
    -DLWIP_DISABLE_PBUF_POOL_SIZE_SANITY_CHECKS=1 \
    -DCHECKSUM_GEN_ICMP6=1 \
    -DCHECKSUM_CHECK_ICMP6=1 \
    -DLWIP_TIMEVAL_PRIVATE=0 \
    -DconfigAPPLICATION_ALLOCATED_HEAP=1 \
    -DUSB_STACK_FREERTOS \
    -DSDK_OS_FREE_RTOS \
    -mcpu=cortex-m7 \
    -Wall \
    -mthumb \
    -MMD \
    -MP \
    -fno-common \
    -ffunction-sections \
    -fdata-sections \
    -fno-builtin \
    -mapcs \
    -std=gnu99 \
    ${FPU} \
    ${DEBUG_CONSOLE_CONFIG} \
")
ENDIF()

IF(NOT DEFINED CMAKE_CXX_FLAGS_BASE)  
SET(CMAKE_CXX_FLAGS_BASE " \
    -DCPU_MIMXRT1176DVMAA_cm7 \
    -DMCUXPRESSO_SDK \
    -DSERIAL_PORT_TYPE_UART=1 \
    -mcpu=cortex-m7 \
    -Wall \
    -mthumb \
    -MMD \
    -MP \
    -fno-common \
    -ffunction-sections \
    -fdata-sections \
    -fno-builtin \
    -mapcs \
    -fno-rtti \
    -fno-exceptions \
    ${FPU} \
    ${DEBUG_CONSOLE_CONFIG} \
")
ENDIF()

IF(NOT DEFINED CMAKE_EXE_LINKER_FLAGS_BASE)  
SET(CMAKE_EXE_LINKER_FLAGS_BASE " \
    -mcpu=cortex-m7 \
    -Wall \
    -fno-common \
    -ffunction-sections \
    -fdata-sections \
    -fno-builtin \
    -fno-lto\
    -mthumb \
    -mapcs \
    -Xlinker --gc-sections \
    -Xlinker -static \
    -Xlinker -z \
    -Xlinker muldefs \
    -Xlinker -Map=output.map \
    -Wl,--print-memory-usage \
    ${FPU} \
    ${SPECS} \
")
ENDIF()