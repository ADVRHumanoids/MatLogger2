cmake_minimum_required(VERSION 3.5)

project(matlogger2 VERSION 1.0.0 LANGUAGES CXX)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

include_directories(
    include/
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}/matio-cmake/matio/src/
    ${CMAKE_CURRENT_BINARY_DIR}/matio-cmake/
    ${CMAKE_CURRENT_BINARY_DIR}/
    src/
)

# Compile matio
add_subdirectory(matio-cmake/)
 
# Compile and install matlogger2 library
set(LIBRARY_TARGET_NAME matlogger2) # set library name
set(${LIBRARY_TARGET_NAME}_SRC 
        src/matlogger2.cpp
        src/mat_appender.cpp
        src/matlogger2_backend.cpp
        src/var_buffer.cpp
    )
add_library(${LIBRARY_TARGET_NAME} SHARED ${${LIBRARY_TARGET_NAME}_SRC})
target_link_libraries(${LIBRARY_TARGET_NAME} PRIVATE dl -pthread)
target_compile_options(${LIBRARY_TARGET_NAME} PRIVATE -std=c++14)
set_target_properties(${LIBRARY_TARGET_NAME} PROPERTIES 
        VERSION ${${PROJECT_NAME}_VERSION})
        
target_include_directories(${LIBRARY_TARGET_NAME} INTERFACE "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
                                                            "$<INSTALL_INTERFACE:$<INSTALL_PREFIX>/${CMAKE_INSTALL_INCLUDEDIR}>")

# Specify installation targets, typology and destination folders.
install(TARGETS  ${LIBRARY_TARGET_NAME}
        EXPORT   ${LIBRARY_TARGET_NAME}
        LIBRARY  DESTINATION "${CMAKE_INSTALL_LIBDIR}" COMPONENT shlib
        ARCHIVE  DESTINATION "${CMAKE_INSTALL_LIBDIR}" COMPONENT lib
        RUNTIME  DESTINATION "${CMAKE_INSTALL_BINDIR}" COMPONENT bin)

# Mark cpp header files for installation
install(DIRECTORY include/${PROJECT_NAME}/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${PROJECT_NAME}
    FILES_MATCHING PATTERN "*.h*"
    PATTERN ".svn" EXCLUDE
)

include(InstallBasicPackageFiles)
install_basic_package_files(${LIBRARY_TARGET_NAME}
                            VERSION ${${PROJECT_NAME}_VERSION}
                            COMPATIBILITY AnyNewerVersion
                            EXPORT ${LIBRARY_TARGET_NAME}
                            VARS_PREFIX ${LIBRARY_TARGET_NAME}
                            NO_CHECK_REQUIRED_COMPONENTS_MACRO)
                            

# if Xenomai is found, compile the -xeno target to be used on RT systems
option(ENABLE_XENO "Compile and install matlogger2-xeno target" FALSE)
if(${ENABLE_XENO})
    include(cmake/xeno-build.cmake)
endif()
             
             
# Compile and install matio backend  
set(MATIO_BACKEND_NAME matlogger2-backend-matio)
add_library(${MATIO_BACKEND_NAME} SHARED src/matio_backend.cpp)
target_compile_options(${MATIO_BACKEND_NAME} PRIVATE -std=c++14)
target_link_libraries(${MATIO_BACKEND_NAME} PRIVATE matio)
set_target_properties(${MATIO_BACKEND_NAME} PROPERTIES 
        VERSION ${${PROJECT_NAME}_VERSION})
# Specify installation targets, typology and destination folders.
install(TARGETS  ${MATIO_BACKEND_NAME}
        EXPORT   ${LIBRARY_TARGET_NAME}
        LIBRARY  DESTINATION "${CMAKE_INSTALL_LIBDIR}"  COMPONENT shlib
        ARCHIVE  DESTINATION "${CMAKE_INSTALL_LIBDIR}"  COMPONENT lib
        RUNTIME  DESTINATION "${CMAKE_INSTALL_BINDIR}"  COMPONENT bin)
     
                            
# Add the uninstall target
include(AddUninstallTarget)

# Generate .deb
include(cmake/generate_deb_pkg.cmake)

# Add Testing target 
option(BUILD_TESTS "Compile ${PROJECT_NAME} tests" FALSE)

if(BUILD_TESTS)
  enable_testing()
  add_custom_target(test_verbose ${CMAKE_CTEST_COMMAND} -V)
  add_subdirectory(tests)
endif()



