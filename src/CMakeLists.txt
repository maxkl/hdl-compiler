
unset(sources)

add_subdirectory("shared/")
add_subdirectory("frontend/")
add_subdirectory("backend/")

add_executable(hdlc
	${sources}
	"main.c"
)
