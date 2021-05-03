if("${USER_BITS}" STREQUAL "")
    set(USER_BITS 64)
endif()

# VERSION DEFINES

file(STRINGS ${CMAKE_CURRENT_LIST_DIR}/wd_ver.h wd_ver)
string(REGEX MATCH "WD_MAJOR_VER ([0-9]*)" _ ${wd_ver})
set(WD_MAJOR_VER ${CMAKE_MATCH_1})
string(REGEX MATCH "WD_MINOR_VER ([0-9]*)" _ ${wd_ver})
set(WD_MINOR_VER ${CMAKE_MATCH_1})
string(REGEX MATCH "WD_SUB_MINOR_VER ([0-9]*)" _ ${wd_ver})
set(WD_SUB_MINOR_VER ${CMAKE_MATCH_1})
string(CONCAT WD_VERSION "${WD_MAJOR_VER}${WD_MINOR_VER}${WD_SUB_MINOR_VER}")
string(CONCAT WD_VERSION_DOTS "${WD_MAJOR_VER}.${WD_MINOR_VER}.${WD_SUB_MINOR_VER}")

set(WD_BASEDIR ${CMAKE_CURRENT_LIST_DIR}/..)
# Populate driver name
set(DRIVER_NAME windrvr${WD_VERSION})
set(SERVICE_NAME WinDriver${WD_VERSION})
set(WDHWID *WINDRVR${WD_VERSION})

# SHARED DEPENDENCIES
find_package(Threads)

set (SAMPLE_SHARED_SRCS
    ${CMAKE_CURRENT_LIST_DIR}/../samples/shared/wdc_diag_lib.c
    ${CMAKE_CURRENT_LIST_DIR}/../samples/shared/diag_lib.c)

# ENVIRONMENT SPECIFIC DEFINES

if (${USER_BITS} STREQUAL 64)
    set(PLAT_DIR "amd64")
else()
    set(PLAT_DIR "x86")
    #add_definitions("-fno-pie")
endif()
message (STATUS "Compiling for ${USER_BITS}-bit")

if (UNIX AND NOT APPLE)
    message (STATUS "Compiling for LINUX")
    set(ARCH LINUX)

    if ("${PLATFORM}" STREQUAL "arm")
        set(KERNEL_BITS 32)
    elseif("${PLATFORM}" STREQUAL "arm64")
        set(KERNEL_BITS 64)
    elseif("${PLATFORM}" STREQUAL "x86")
        set(KERNEL_BITS 32)
    else()
        set(KERNEL_BITS 64)
        set(PLATFORM x86_64)
    endif()

    if (${USER_BITS} STREQUAL 32 AND ${KERNEL_BITS} STREQUAL 64)
        set(WDAPI_LIB "wdapi${WD_VERSION}_32")
    else()
        set(WDAPI_LIB "wdapi${WD_VERSION}")
    endif()

    if ( ${PLATFORM} STREQUAL "x86_64")
        add_definitions("-m64")
        set(KERNEL_FLAGS "-march=k8 -mno-red-zone -mcmodel=kernel
            -fno-reorder-blocks -Wno-sign-compare
            -fno-asynchronous-unwind-tables -fPIC -fno-pie -funit-at-a-time")
    elseif(${PLATFORM} STREQUAL "x86")
        set(KERNEL_FLAGS "-mpreferred-stack-boundary=2 -mregparm=3
            -fno-stack-protector")
    endif()

    if (${PLATFORM} STREQUAL "x86_64")
        set(ARCH_KERNEL_LFLAG -melf_x86_64)
    else()
        set(ARCH_LFLAG -fno-pie)
    endif()

    add_link_options(${ARCH_LFLAG})
    add_definitions("-Wno-unused-result -Wno-write-strings ")

    # CLEANUP
    set_property(DIRECTORY APPEND PROPERTY ADDITIONAL_MAKE_CLEAN_FILES "${ARCH}/*")

endif()

if (WIN32)
    message (STATUS "Compiling for WINDOWS")
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
    set(WDAPI_LIB wdapi${WD_VERSION})
    if (DEFINED ENV{PROCESSOR_ARCHITEW6432} OR
        ${CMAKE_HOST_SYSTEM_PROCESSOR} STREQUAL "AMD64")
        set(KERNEL_BITS 64)
    else()
        set(KERNEL_BITS 32)
    endif()
    set(ARCH WIN32)
    if (${USER_BITS} STREQUAL 32 AND ${KERNEL_BITS} STREQUAL 64)
        set(WDAPI_LIB
            "${WD_BASEDIR}/lib/${PLAT_DIR}/x86/wdapi${WD_VERSION}_32.lib")
    else()
        set(WDAPI_LIB "${WD_BASEDIR}/lib/${PLAT_DIR}/wdapi${WD_VERSION}.lib")
    endif()
endif()

message(STATUS "${KERNEL_BITS} Bit Kernel Detected")
if (${PLATFORM} STREQUAL "arm")
    message("ARM compilation")
elseif(${PLATFORM} STREQUAL "arm64")
    add_definitions(-DKERNEL_64BIT)
    message("ARM64 compilation")
elseif(${PLATFORM} STREQUAL "x86_64")
    add_definitions(-DKERNEL_64BIT -Dx86_64)
else()
    add_definitions(-Dx86 -Di386)
endif()

add_definitions(-D${ARCH} -DWD_DRIVER_NAME_CHANGE)
