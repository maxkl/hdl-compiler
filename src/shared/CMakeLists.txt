
unset(sources)

add_subdirectory("helper/")

add_library(shared
	${sources}
	"intermediate_file.c"
	"intermediate_file.h"
)
