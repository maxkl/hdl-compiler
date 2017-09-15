
add_executable(frontend
	"main.c"
	"lexer.c"
	"lexer.h"
	"parser.c"
	"parser.h"
	"ast.c"
	"ast.h"
)
target_link_libraries(frontend shared)
