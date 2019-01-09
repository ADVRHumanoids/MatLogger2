configure_file(
  "${PROJECT_SOURCE_DIR}/cmake/matio_pubconf.cmake.in"
  "${CMAKE_CURRENT_BINARY_DIR}/matio/src/matio_pubconf.h"
  ESCAPE_QUOTES @ONLY)

configure_file(
  "${PROJECT_SOURCE_DIR}/cmake/matioConfig.cmake.in"
  "${CMAKE_CURRENT_BINARY_DIR}/matio/src/matioConfig.h"
  ESCAPE_QUOTES @ONLY)

set(src_SOURCES
  ${PROJECT_SOURCE_DIR}/matio/src/endian.cpp
  ${PROJECT_SOURCE_DIR}/matio/src/mat.cpp
  ${PROJECT_SOURCE_DIR}/matio/src/io.cpp
  ${PROJECT_SOURCE_DIR}/matio/src/inflate.cpp
  ${PROJECT_SOURCE_DIR}/matio/src/mat73.cpp
  ${PROJECT_SOURCE_DIR}/matio/src/matvar_cell.cpp
  ${PROJECT_SOURCE_DIR}/matio/src/matvar_struct.cpp
  ${PROJECT_SOURCE_DIR}/matio/src/mat4.cpp
  ${PROJECT_SOURCE_DIR}/matio/src/mat5.cpp
  ${PROJECT_SOURCE_DIR}/matio/src/snprintf.cpp
  ${PROJECT_SOURCE_DIR}/matio/src/read_data.cpp
#   ${PROJECT_SOURCE_DIR}/matio/src/mat5.h
#   ${PROJECT_SOURCE_DIR}/matio/src/mat73.h
#   ${PROJECT_SOURCE_DIR}/matio/src/matio_private.h
#   ${PROJECT_SOURCE_DIR}/matio/src/mat4.h
#   ${PROJECT_SOURCE_DIR}/matio/src/matio.h
#   ${CMAKE_CURRENT_BINARY_DIR}/matio/src/matio_pubconf.h
#   ${CMAKE_CURRENT_BINARY_DIR}/matio/src/matioConfig.h
)

set_source_files_properties(${src_SOURCES} PROPERTIES LANGUAGE CXX )

add_compile_options("-fPIC")
add_compile_options("-std=c++14")

add_library(matio STATIC ${src_SOURCES})
target_include_directories(matio
    PRIVATE ${PROJECT_SOURCE_DIR}/matio/src/
    PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/matio/src/
)


if(NOT WIN32)
  target_link_libraries(matio PUBLIC m)
else()
  target_link_libraries(matio PUBLIC ${GETOPT_LIB})
  set_target_properties(matio PROPERTIES OUTPUT_NAME libmatio)
endif()

if(HDF5_FOUND)
  target_link_libraries(matio
    PUBLIC HDF5::HDF5)
endif()

if(ZLIB_FOUND)
  target_link_libraries(matio
      PUBLIC ZLIB::ZLIB
  )
endif()

# XXX not sure it's the right thing to do...
# set_target_properties(matio PROPERTIES
#   CXX_STANDARD_REQUIRED ON
#   CXX_VISIBILITY_PRESET hidden
#   VISIBILITY_INLINES_HIDDEN 1)


# This generates matio_export.h
include(GenerateExportHeader)
generate_export_header(matio)

set_target_properties(matio PROPERTIES PUBLIC_HEADER "${PROJECT_SOURCE_DIR}/matio/src/matio.h;${CMAKE_CURRENT_BINARY_DIR}/matio/src/matio_pubconf.h;${CMAKE_CURRENT_BINARY_DIR}/matio_export.h") # XXX: check whether matio_pubconf.h or matioConfig.h is the current strategy (one of the two is deprected)


# 'make install' to the correct locations (provided by GNUInstallDirs).
# install(TARGETS matio EXPORT libmatio
#         PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
#         RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
#         LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
#         ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
# install(EXPORT libmatio NAMESPACE matio:: DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake)
