cmake_minimum_required(VERSION 3.5)

project(matlogger2 VERSION 1.3.1 LANGUAGES CXX)

# Loading settings from external projects

find_package(Eigen3 REQUIRED)

find_package(Boost REQUIRED)

# Environment variables

set(CMAKE_BUILD_RPATH "$ORIGIN")

set(LIBRARY_TARGET_NAME matlogger2) # set library name

set(${LIBRARY_TARGET_NAME}_SRC 
        src/matlogger2.cpp
        src/mat_appender.cpp
        src/matlogger2_backend.cpp
        src/var_buffer.cpp
        src/mat_data.cpp
)

set(LIB_EXT ".so")
if(APPLE)
    set(LIB_EXT ".dylib")
endif()

set(MATIO_BACKEND_NAME matlogger2-backend-matio)

# List operations

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

# Options
option(MATLOGGER2_VERBOSE "Enable debug prints" OFF)
option(COMPILE_PY_BINDINGS "Compile Python bindings" TRUE)
option(GEN_DEB "Generate .deb" TRUE)
option(GEN_DOCS "Generate docs" TRUE)
option(BUILD_TESTS "Compile ${PROJECT_NAME} tests" FALSE)
option(ENABLE_XENO "Compile and install matlogger2-xeno target" FALSE)

# Adding include directories

include_directories(
    include/
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}/matio/src/
    ${CMAKE_CURRENT_BINARY_DIR}/
    ${EIGEN3_INCLUDE_DIR}
    ${Boost_INCLUDE_DIRS}
    src/
)

# Adding subdirectories to the build
set(MATIO_SHARED OFF) # compile matio as a static library
add_subdirectory(matio/) # matio backend library

if(COMPILE_PY_BINDINGS)
  add_subdirectory(bindings/python/) # python bindings
endif()

if(GEN_DOCS)
  add_subdirectory(doc/) # docs
endif()

if(BUILD_TESTS)
  add_subdirectory(tests) # tests
endif()

# Adding libraries to the project

add_library(${LIBRARY_TARGET_NAME} SHARED ${${LIBRARY_TARGET_NAME}_SRC})

add_library(${MATIO_BACKEND_NAME} SHARED src/matio_backend.cpp)

# Linking targets

target_link_libraries(${LIBRARY_TARGET_NAME} PRIVATE dl -pthread)

target_link_libraries(${MATIO_BACKEND_NAME} PRIVATE matio)

# Adding target compile options

target_compile_options(${LIBRARY_TARGET_NAME} PRIVATE -std=c++14)

target_compile_options(${MATIO_BACKEND_NAME} PRIVATE -std=c++14)

# Setting target custom properties

set_target_properties(${LIBRARY_TARGET_NAME} PROPERTIES 
        VERSION ${${PROJECT_NAME}_VERSION})

set_target_properties(${MATIO_BACKEND_NAME} PROPERTIES 
        VERSION ${${PROJECT_NAME}_VERSION})

# Compile definitions

target_compile_definitions(${LIBRARY_TARGET_NAME} PRIVATE -DMATLOGGER2_LIB_EXT="${LIB_EXT}")

# Target include directories

target_include_directories(${LIBRARY_TARGET_NAME} INTERFACE "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
                                                            "$<INSTALL_INTERFACE:$<INSTALL_PREFIX>/${CMAKE_INSTALL_INCLUDEDIR}>"
                                                            ${EIGEN3_INCLUDE_DIR})
                                                     
# Including cmake files

include(InstallBasicPackageFiles) # helper module to make the package easier to be found by other projects

if(${ENABLE_XENO})
    include(cmake/xeno-build.cmake) # if Xenomai is found, compile the -xeno target to be used on RT systems
endif()

if(GEN_DEB)
  include(cmake/generate_deb_pkg.cmake) # generate .deb
endif()

if(BUILD_TESTS) 
  enable_testing()
  add_custom_target(test_verbose ${CMAKE_CTEST_COMMAND} -V)
endif()

# Add the uninstall target

include(AddUninstallTarget)

# Specify installation targets, typology and destination folders.

install(TARGETS  ${LIBRARY_TARGET_NAME}
        EXPORT   ${LIBRARY_TARGET_NAME}
        LIBRARY  DESTINATION "${CMAKE_INSTALL_LIBDIR}" COMPONENT shlib
        ARCHIVE  DESTINATION "${CMAKE_INSTALL_LIBDIR}" COMPONENT lib
        RUNTIME  DESTINATION "${CMAKE_INSTALL_BINDIR}" COMPONENT bin) # sources

install(TARGETS  ${MATIO_BACKEND_NAME}
        EXPORT   ${LIBRARY_TARGET_NAME}
        LIBRARY  DESTINATION "${CMAKE_INSTALL_LIBDIR}"  COMPONENT shlib
        ARCHIVE  DESTINATION "${CMAKE_INSTALL_LIBDIR}"  COMPONENT lib
        RUNTIME  DESTINATION "${CMAKE_INSTALL_BINDIR}"  COMPONENT bin) # matio backend

install(DIRECTORY include/${PROJECT_NAME}/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${PROJECT_NAME}
    FILES_MATCHING PATTERN "*.h*"
    PATTERN ".svn" EXCLUDE) # Mark cpp header files for installation

install_basic_package_files(${LIBRARY_TARGET_NAME}
                            VERSION ${${PROJECT_NAME}_VERSION}
                            COMPATIBILITY AnyNewerVersion
                            EXPORT ${LIBRARY_TARGET_NAME}
                            VARS_PREFIX ${LIBRARY_TARGET_NAME}
                            NO_CHECK_REQUIRED_COMPONENTS_MACRO)
                                      
