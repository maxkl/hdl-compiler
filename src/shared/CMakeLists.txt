
add_subdirectory("helper/")

set(sources ${sources}
	"${CMAKE_CURRENT_SOURCE_DIR}/intermediate.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/intermediate.h"
	"${CMAKE_CURRENT_SOURCE_DIR}/intermediate_file.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/intermediate_file.h"
)

set(sources ${sources} PARENT_SCOPE)
