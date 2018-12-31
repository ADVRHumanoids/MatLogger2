
find_package(Threads)

find_package(HDF5 REQUIRED)

find_package(ZLIB REQUIRED)

message(STATUS "HDF5 found ${HDF5_FOUND}")
message(STATUS "ZLIB found ${ZLIB_FOUND}")

# FindHDF5.cmake does not expose a modern CMake Target

if (HDF5_FOUND AND NOT TARGET HDF5::HDF5)
    message(STATUS "Generating HDF5 target ${HDF5_LIBRARIES}")
    add_library(HDF5::HDF5 INTERFACE IMPORTED)
    set_target_properties(HDF5::HDF5 PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${HDF5_INCLUDE_DIRS}"
        INTERFACE_LINK_LIBRARIES "${HDF5_LIBRARIES}")
endif()
