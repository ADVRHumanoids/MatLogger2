cmake_minimum_required(VERSION 3.5)
include(ExternalProject)

project(matlogger2_tests LANGUAGES C CXX VERSION 1.0.0)

add_compile_options("-std=c++14")
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
    set(GTEST_EXT_TARGET GTest-ext)
endif()

set(TestLibs matlogger2 ${GTEST_BOTH_LIBRARIES} -lm -lpthread matio)

include_directories(${GTEST_INCLUDE_DIRS})
link_directories(${GTEST_LIB_DIRS})

add_executable(TestApi TestApi.cpp)
add_executable(ProfileTest profile_test.cpp)
add_executable(BackendTest BackendTest.cpp)
add_executable(ReadTests ReadTests.cpp)

target_link_libraries(TestApi ${TestLibs}) # -fsanitize=thread)
target_link_libraries(ProfileTest matlogger2 -lpthread)
target_link_libraries(BackendTest ${TestLibs} ) # -fsanitize=thread)
target_link_libraries(ReadTests ${TestLibs} ) 

add_dependencies(TestApi ${GTEST_EXT_TARGET} matlogger2)
add_dependencies(BackendTest ${GTEST_EXT_TARGET} matlogger2)
add_dependencies(ReadTests ${GTEST_EXT_TARGET} matlogger2)

add_test(NAME TestApi COMMAND TestApi)
add_test(NAME BackendTest COMMAND BackendTest)
add_test(NAME ReadTests COMMAND ReadTests)