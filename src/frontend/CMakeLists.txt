
set(sources ${sources}
	"${CMAKE_CURRENT_SOURCE_DIR}/frontend.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/frontend.h"
	"${CMAKE_CURRENT_SOURCE_DIR}/lexer.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/lexer.h"
	"${CMAKE_CURRENT_SOURCE_DIR}/parser.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/parser.h"
	"${CMAKE_CURRENT_SOURCE_DIR}/ast.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/ast.h"
	"${CMAKE_CURRENT_SOURCE_DIR}/symbol.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/symbol.h"
	"${CMAKE_CURRENT_SOURCE_DIR}/type_symbol.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/type_symbol.h"
	"${CMAKE_CURRENT_SOURCE_DIR}/symbol_table.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/symbol_table.h"
	"${CMAKE_CURRENT_SOURCE_DIR}/analyzer.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/analyzer.h"
	"${CMAKE_CURRENT_SOURCE_DIR}/intermediate_generator.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/intermediate_generator.h"
	"${CMAKE_CURRENT_SOURCE_DIR}/expression_type.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/expression_type.h"
)

set(sources ${sources} PARENT_SCOPE)
