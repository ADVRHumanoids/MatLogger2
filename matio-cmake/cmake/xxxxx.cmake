set(MATIO_PLATFORM ${CMAKE_SYSTEM_PROCESSOR}-unknown-${CMAKE_SYSTEM_NAME})

include(cmake/utils/VA_COPY.cmake)
VA_COPY()

if (NOT HAVE_GETOPT)
    add_library(gnu STATIC ${PROJECT_SOURCE_DIR}/matio/getopt/getopt_long.c)
    target_compile_definitions(gnu
        PUBLIC -DREPLACE_GETOPT)
    target_include_directories(gnu
        PUBLIC ${PROJECT_SOURCE_DIR}/matio/getopt/)
    set(GETOPT_LIB gnu)
endif()


include(cmake/utils/CheckHeaderSTDC.cmake)
check_header_stdc()

# Make the variables HAVE_MAT_UINT8_T, etc...
set(TYPES uint8_t uint16_t uint32_t uint64_t int8_t int16_t int32_t int64_t)
foreach(TYPE ${TYPES})
    string(TOUPPER ${TYPE} TYPE_UPPER)
    check_type_size(${TYPE} ${TYPE_UPPER})
    set(HAVE_MAT_${TYPE_UPPER} ${HAVE_${TYPE_UPPER}})
    set(SIZEOF_${TYPE_UPPER} ${${TYPE_UPPER}})
    if (NOT ${${TYPE_UPPER}})
        message(FATAL_ERROR "Unknown type ${TYPE}")
    endif()
endforeach()


# Setting the include directory for the application to find config.h
include_directories( ${CMAKE_BINARY_DIR} )
# Since we have created a config.h add a global define for it
add_definitions( "-DHAVE_CONFIG_H" )
