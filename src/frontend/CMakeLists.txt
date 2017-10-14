
add_executable(frontend
	"main.c"
	"lexer.c"
	"lexer.h"
	"parser.c"
	"parser.h"
	"ast.c"
	"ast.h"
	"symbol.c"
	"symbol.h"
	"symbol_table.c"
	"symbol_table.h"
	"analyzer.c"
	"analyzer.h"
	"intermediate_generator.c"
	"intermediate_generator.h"
	"expression_type.c"
	"expression_type.h"
)
target_link_libraries(frontend shared)
