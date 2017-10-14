
add_subdirectory("LogicSimulator/")
add_subdirectory("linker/")

add_executable(backend
	"main.c"
)
target_link_libraries(backend shared)
