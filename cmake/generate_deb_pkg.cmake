set(CPACK_PACKAGING_INSTALL_PREFIX "/usr/local" CACHE PATH "Deb package install prefix")
set(CPACK_GENERATOR "DEB")
set(CPACK_PACKAGE_NAME ${PROJECT_NAME})
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "amd64")
set(CPACK_PACKAGE_ARCHITECTURE "amd64")
set(CPACK_DEBIAN_PACKAGE_DEPENDS "")
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "arturo.laurenzi@iit.it")
set(CPACK_DEBIAN_PACKAGE_DESCRIPTION "C++ library for logging numeric data to MAT-file")

find_program(GIT_SCM git DOC "Git version control")
mark_as_advanced(GIT_SCM)
find_file(GITDIR NAMES .git PATHS ${CMAKE_SOURCE_DIR} NO_DEFAULT_PATH)
if (GIT_SCM AND GITDIR)
    execute_process(
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMAND ${GIT_SCM} log -1 "--pretty=format:%h"
        OUTPUT_VARIABLE GIT_SHA1_SHORT
    )
else()
    # No version control
    # e.g. when the software is built from a source tarball
    # and gitrevision.hh is packaged with it but no Git is available
    message(STATUS "Will not remake ${SRCDIR}/gitrevision.hh")
endif()

execute_process(
    COMMAND lsb_release -cs
    OUTPUT_VARIABLE LINUX_DISTRO_NAME
)

string(REPLACE "\n" "" LINUX_DISTRO_NAME ${LINUX_DISTRO_NAME})

set(CPACK_PACKAGE_FILE_NAME ${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}-${GIT_SHA1_SHORT}-${CPACK_PACKAGE_ARCHITECTURE}-${LINUX_DISTRO_NAME})

message(STATUS "Will generate package '${CPACK_PACKAGE_FILE_NAME}.deb'")

include(CPack)
