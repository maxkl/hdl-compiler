
unset(sources)

add_subdirectory("shared/")
add_subdirectory("frontend/")
add_subdirectory("linker/")
add_subdirectory("backend/")

add_executable(hdlc
	${sources}
	"main.cpp"
)
target_link_libraries(hdlc PRIVATE nlohmann_json::nlohmann_json)
target_link_libraries(hdlc PRIVATE unwind)
