# CROSS COMPILER SETTING
SET(CMAKE_SYSTEM_NAME Generic)
CMAKE_MINIMUM_REQUIRED (VERSION 3.10.0)

SET(SOFTWARE_VERSION "1.5.6")

# THE VERSION NUMBER
SET (MCUXPRESSO_CMAKE_FORMAT_MAJOR_VERSION 2)
SET (MCUXPRESSO_CMAKE_FORMAT_MINOR_VERSION 0)

include(ide_overrides.cmake OPTIONAL)

if(CMAKE_SCRIPT_MODE_FILE)
  message("${MCUXPRESSO_CMAKE_FORMAT_MAJOR_VERSION}")
  return()
endif()

# ENABLE ASM
ENABLE_LANGUAGE(ASM)

add_compile_options(-Wno-int-conversion -Wno-implicit-function-declaration)

SET(CMAKE_STATIC_LIBRARY_PREFIX)
SET(CMAKE_STATIC_LIBRARY_SUFFIX)

SET(CMAKE_EXECUTABLE_LIBRARY_PREFIX)
SET(CMAKE_EXECUTABLE_LIBRARY_SUFFIX)


# CURRENT DIRECTORY
SET(ProjDirPath ${CMAKE_CURRENT_SOURCE_DIR})
SET(SdkBoard "evkmimxrt1170")

if (DEFINED ENV{SdkRootDirPath})
    SET(SdkRootDirPath "$ENV{SdkRootDirPath}")
else()
    SET(SdkRootDirPath "${ProjDirPath}/../../../../../../..")
endif()

# check if PLATFORM environment variable is defined
if (DEFINED ENV{PROJECT})
    SET(ProjectName "$ENV{PROJECT}")
else()
    message(FATAL_ERROR "PROJECT environment variable is not defined")
endif()

# check if BOARD environment variable is defined
if (NOT DEFINED ENV{BOARD})
    message(FATAL_ERROR "BOARD environment variable is not defined")
endif()

# check if TYPE environment variable is defined and convert it to integer
if (NOT DEFINED ENV{TYPE})
    message(FATAL_ERROR "TYPE environment variable is not defined")
endif()

SET(EXECUTABLE_OUTPUT_PATH ${ProjDirPath}/${ProjectName}_${CMAKE_BUILD_TYPE})
SET(LIBRARY_OUTPUT_PATH ${ProjDirPath}/${ProjectName}_${CMAKE_BUILD_TYPE})

project(${ProjectName})
add_definitions(-DSOFTWARE_VERSION="${SOFTWARE_VERSION}")

set(MCUX_BUILD_TYPES debug release flexspi_nor_sdram_debug)

set(MCUX_SDK_PROJECT_NAME ${ProjectName}_${CMAKE_BUILD_TYPE}.elf)

if (NOT DEFINED SdkRootDirPath)
    SET(SdkRootDirPath ${SdkRootDirPath})
endif()

# suppress ALL warnings
add_definitions(-w)
if ($ENV{BOARD} STREQUAL "EVK")
    add_definitions(-DBOARD_EVK)
elseif ($ENV{BOARD} STREQUAL "ALON_CARRIER")
    add_definitions(-DBOARD_ALON_CARRIER)
elseif ($ENV{BOARD} STREQUAL "TVAI_CARRIER")
    add_definitions(-DBOARD_TVAI_CARRIER)
else()
    message(FATAL_ERROR "Unknown board $ENV{BOARD}")
endif()

string(TOLOWER "$ENV{BOARD}" ProjectBoard)

include(${ProjDirPath}/flags.cmake)
include(${ProjDirPath}/config.cmake)

# check if project folder exists
if(NOT EXISTS ${ProjDirPath}/../projects/${ProjectName})
    message(FATAL_ERROR "Project folder does not exist ${ProjDirPath}/../projects/${ProjectName}")
endif()

# check if board folder exists
if(NOT EXISTS ${ProjDirPath}/../boards/${ProjectBoard})
    message(FATAL_ERROR "Board folder does not exist ${ProjDirPath}/../boards/${ProjectBoard}")
endif()


file(GLOB_RECURSE BOARD_HEADERS "${ProjDirPath}/../boards/${ProjectBoard}/*.h")
file(GLOB_RECURSE BOARD_SOURCES "${ProjDirPath}/../boards/${ProjectBoard}/*.c")

file(GLOB_RECURSE PROJECT_HEADERS "${ProjDirPath}/../projects/${ProjectName}/*.h")
file(GLOB_RECURSE PROJECT_SOURCES "${ProjDirPath}/../projects/${ProjectName}/*.c")
file(GLOB_RECURSE PROJECT_SOURCES_CPP "${ProjDirPath}/../projects/${ProjectName}/*.cpp")

file(GLOB_RECURSE SRC_HEADERS "${ProjDirPath}/../src/*.h")
file(GLOB_RECURSE SRC_SOURCES "${ProjDirPath}/../src/*.c")
file(GLOB_RECURSE SRC_SOURCES_CPP "${ProjDirPath}/../src/*.cpp")

add_executable(${MCUX_SDK_PROJECT_NAME}
# "${SdkRootDirPath}/core/boards/${SdkBoard}/board.c"
# "${SdkRootDirPath}/core/boards/${SdkBoard}/board.h"
"${SdkRootDirPath}/core/boards/${SdkBoard}/clock_config.c"
"${SdkRootDirPath}/core/boards/${SdkBoard}/clock_config.h"
"${SdkRootDirPath}/core/boards/${SdkBoard}/dcd.c"
"${SdkRootDirPath}/core/boards/${SdkBoard}/dcd.h"
"${ProjDirPath}/../${SdkBoard}_connect_cm4_cm7side.jlinkscript"
${BOARD_HEADERS}
${BOARD_SOURCES}
${PROJECT_SOURCES}
${PROJECT_SOURCES_CPP}
${SRC_HEADERS}
${SRC_SOURCES}
${SRC_SOURCES_CPP}
)

target_include_directories(${MCUX_SDK_PROJECT_NAME} PRIVATE
    ${ProjDirPath}/../boards/${ProjectBoard}
    ${ProjDirPath}/../projects/${ProjectName}
    ${ProjDirPath}/../src
    ${SdkRootDirPath}/core/boards/${SdkBoard}
)

set_source_files_properties("${ProjDirPath}/../src/usb_host_config.h" PROPERTIES COMPONENT_CONFIG_FILE "middleware_usb_host_ehci_config_header")
set_source_files_properties("${ProjDirPath}/../src/lwipopts.h" PROPERTIES COMPONENT_CONFIG_FILE "middleware_lwip_template")
set_source_files_properties("${ProjDirPath}/../src/FreeRTOSConfig.h" PROPERTIES COMPONENT_CONFIG_FILE "middleware_freertos-kernel_template")


include(${SdkRootDirPath}/core/devices/MIMXRT1176/all_lib_device.cmake)

IF(NOT DEFINED TARGET_LINK_SYSTEM_LIBRARIES)  
    SET(TARGET_LINK_SYSTEM_LIBRARIES "-lm -lc -lgcc -lnosys")  
ENDIF()  

TARGET_LINK_LIBRARIES(${MCUX_SDK_PROJECT_NAME} PRIVATE -Wl,--start-group)

TARGET_LINK_LIBRARIES(${MCUX_SDK_PROJECT_NAME} PRIVATE ${TARGET_LINK_SYSTEM_LIBRARIES})

TARGET_LINK_LIBRARIES(${MCUX_SDK_PROJECT_NAME} PRIVATE -Wl,--end-group)

SET_TARGET_PROPERTIES(${MCUX_SDK_PROJECT_NAME} PROPERTIES ADDITIONAL_CLEAN_FILES "output.map")

# Add a post-build command to convert ELF to BIN
add_custom_command(TARGET ${MCUX_SDK_PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:${MCUX_SDK_PROJECT_NAME}> ${EXECUTABLE_OUTPUT_PATH}/${ProjectName}_${CMAKE_BUILD_TYPE}_${SOFTWARE_VERSION}.bin
    COMMAND python ${ProjDirPath}/append_type.py $ENV{TYPE} "${ProjectName}_${CMAKE_BUILD_TYPE}_${SOFTWARE_VERSION}.bin"
    WORKING_DIRECTORY ${EXECUTABLE_OUTPUT_PATH} 
    COMMENT "Converting ELF to BIN..."
)