cmake_minimum_required(VERSION 3.5)

find_package(pybind11 QUIET)

add_compile_options(-std=c++14)

if(${pybind11_FOUND})
    
    message(STATUS "Will compile python bindings")
    include_directories(${CMAKE_CURRENT_SOURCE_DIR})
    pybind11_add_module(matlogger pyMatLogger2.cpp)
    target_link_libraries(matlogger PUBLIC matlogger2)

    
    # get python install path 
    execute_process(
        COMMAND ${PYTHON_EXECUTABLE} -c "if True:
            from distutils import sysconfig as sc
            print(sc.get_python_lib(prefix='', plat_specific=True))"
        OUTPUT_VARIABLE PYTHON_SITE
        OUTPUT_STRIP_TRAILING_WHITESPACE)
    message(STATUS "Python install dir: ${PYTHON_SITE}")
    
    install(TARGETS matlogger
            DESTINATION ${PYTHON_SITE}/matlogger2)
            
    file(WRITE ${CMAKE_BINARY_DIR}/__init__.py "")
    install(FILES  ${CMAKE_BINARY_DIR}/__init__.py DESTINATION ${PYTHON_SITE}/matlogger2)

            
else()
    message(STATUS "Pybind11 not found, bindings won't be available")
endif()
