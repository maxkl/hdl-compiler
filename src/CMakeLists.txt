
add_subdirectory("shared/")

add_subdirectory("frontend/")
add_subdirectory("backend/")

add_executable(hdlc
	"main.c"
)
target_link_libraries(hdlc shared)
