cmake_minimum_required(VERSION 3.5)
include(ExternalProject)

project(matlogger2_tests LANGUAGES C CXX VERSION 1.0.0)


add_compile_options("-std=c++11")
# add_compile_options("-fsanitize=thread")

# compite GTest
find_package(GTest)
find_package(Threads)
if(NOT GTEST_FOUND)
    set(gTestSource "/usr/src/gtest")
    ExternalProject_Add(GTest-ext SOURCE_DIR ${gTestSource}
                                  PREFIX "${CMAKE_CURRENT_BINARY_DIR}/external"
                                  INSTALL_COMMAND "")
    set(GTEST_LIB_DIRS "${CMAKE_CURRENT_BINARY_DIR}/external/src/GTest-ext-build/")
    set(GTEST_INCLUDE_DIRS ${gTestSource})
    set(GTEST_BOTH_LIBRARIES gtest gtest_main)
endif()


set(TestLibs matlogger2 ${GTEST_BOTH_LIBRARIES} -lm -lpthread)
include_directories(${GTEST_INCLUDE_DIRS})
link_directories(${GTEST_LIB_DIRS})

add_executable(TestApi TestApi.cpp)
target_link_libraries(TestApi ${TestLibs})
add_dependencies(TestApi GTest-ext matlogger2)
add_test(NAME TestApi COMMAND TestApi)


