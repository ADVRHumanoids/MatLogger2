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

add_subdirectory(matio-cmake/)
 
 
set(LIBRARY_TARGET_NAME matlogger2)

add_library(${LIBRARY_TARGET_NAME} SHARED 
                src/matlogger2.cpp
                src/matlogger_manager.cpp
                )
target_compile_options(${LIBRARY_TARGET_NAME} PRIVATE -std=c++14)
target_link_libraries(${LIBRARY_TARGET_NAME} PRIVATE matio)                                         
set_target_properties(${LIBRARY_TARGET_NAME} PROPERTIES 
        VERSION ${${PROJECT_NAME}_VERSION})
        
target_include_directories(${LIBRARY_TARGET_NAME} INTERFACE "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
                                                         "$<INSTALL_INTERFACE:$<INSTALL_PREFIX>/${CMAKE_INSTALL_INCLUDEDIR}>")

# Specify installation targets, typology and destination folders.
install(TARGETS ${LIBRARY_TARGET_NAME}
        EXPORT  ${LIBRARY_TARGET_NAME}
        LIBRARY       DESTINATION "${CMAKE_INSTALL_LIBDIR}"                            COMPONENT shlib
        ARCHIVE       DESTINATION "${CMAKE_INSTALL_LIBDIR}"                            COMPONENT lib
        RUNTIME       DESTINATION "${CMAKE_INSTALL_BINDIR}"                            COMPONENT bin
        PUBLIC_HEADER DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/${LIBRARY_TARGET_NAME}" COMPONENT dev)


include(InstallBasicPackageFiles)
install_basic_package_files(${LIBRARY_TARGET_NAME}
                            VERSION ${${PROJECT_NAME}_VERSION}
                            COMPATIBILITY AnyNewerVersion
                            EXPORT ${LIBRARY_TARGET_NAME}
                            VARS_PREFIX ${LIBRARY_TARGET_NAME}
                            NO_CHECK_REQUIRED_COMPONENTS_MACRO)
                            
             
find_package(Xenomai QUIET)
if(${Xenomai_FOUND})
    add_library(${LIBRARY_TARGET_NAME}-xeno SHARED 
                    src/matlogger2.cpp
                    src/matlogger_manager.cpp
                    )
    target_compile_options(${LIBRARY_TARGET_NAME}-xeno PRIVATE -std=c++14)
    target_link_libraries(${LIBRARY_TARGET_NAME}-xeno PRIVATE matio)     
    set_xeno_flags(${LIBRARY_TARGET_NAME}-xeno)
    set_target_properties(${LIBRARY_TARGET_NAME}-xeno PROPERTIES 
            VERSION ${${PROJECT_NAME}_VERSION})
            
    target_include_directories(${LIBRARY_TARGET_NAME}-xeno INTERFACE "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
                                                            "$<INSTALL_INTERFACE:$<INSTALL_PREFIX>/${CMAKE_INSTALL_INCLUDEDIR}>")

    # Specify installation targets, typology and destination folders.
    install(TARGETS ${LIBRARY_TARGET_NAME}-xeno
            EXPORT  ${LIBRARY_TARGET_NAME}
            LIBRARY       DESTINATION "${CMAKE_INSTALL_LIBDIR}"                            COMPONENT shlib
            ARCHIVE       DESTINATION "${CMAKE_INSTALL_LIBDIR}"                            COMPONENT lib
            RUNTIME       DESTINATION "${CMAKE_INSTALL_BINDIR}"                            COMPONENT bin
            PUBLIC_HEADER DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/${LIBRARY_TARGET_NAME}" COMPONENT dev)


    include(InstallBasicPackageFiles)
    install_basic_package_files(${LIBRARY_TARGET_NAME}
                                VERSION ${${PROJECT_NAME}_VERSION}
                                COMPATIBILITY AnyNewerVersion
                                EXPORT ${LIBRARY_TARGET_NAME}
                                VARS_PREFIX ${LIBRARY_TARGET_NAME}
                                NO_CHECK_REQUIRED_COMPONENTS_MACRO)
endif()
                            
                            
                            
# Add the uninstall target
include(AddUninstallTarget)


# Add Testing target 
option(BUILD_TESTS "Compile ${PROJECT_NAME} tests" FALSE)

if(BUILD_TESTS)
  enable_testing()
  add_custom_target(test_verbose ${CMAKE_CTEST_COMMAND} -V)
  add_subdirectory(tests)
endif()

