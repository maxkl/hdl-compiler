
add_subdirectory("helper/")

set(sources ${sources}
	"${CMAKE_CURRENT_SOURCE_DIR}/Intermediate.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/Intermediate.h"
	"${CMAKE_CURRENT_SOURCE_DIR}/IntermediateFile.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/IntermediateFile.h"
	"${CMAKE_CURRENT_SOURCE_DIR}/errors.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/errors.h"
)

set(sources ${sources} PARENT_SCOPE)
