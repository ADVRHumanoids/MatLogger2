set(getopt_SOURCES
	${PROJECT_SOURCE_DIR}/matio/getopt/getopt_long.c
	${PROJECT_SOURCE_DIR}/matio/getopt/getopt.h
)
add_library(getopt STATIC ${getopt_SOURCES} )
