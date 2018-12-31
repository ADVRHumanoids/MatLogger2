add_executable(matdump ${PROJECT_SOURCE_DIR}/matio/tools/matdump.c )

target_link_libraries( matdump
    matio
    getopt
)
