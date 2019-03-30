
set(sources ${sources}
	"${CMAKE_CURRENT_SOURCE_DIR}/Frontend.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/Frontend.h"
	"${CMAKE_CURRENT_SOURCE_DIR}/Lexer.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/Lexer.h"
	"${CMAKE_CURRENT_SOURCE_DIR}/Parser.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/Parser.h"
	"${CMAKE_CURRENT_SOURCE_DIR}/AST.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/AST.h"
	"${CMAKE_CURRENT_SOURCE_DIR}/Symbol.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/Symbol.h"
	"${CMAKE_CURRENT_SOURCE_DIR}/TypeSymbol.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/TypeSymbol.h"
	"${CMAKE_CURRENT_SOURCE_DIR}/SymbolTable.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/SymbolTable.h"
	"${CMAKE_CURRENT_SOURCE_DIR}/SemanticAnalyzer.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/SemanticAnalyzer.h"
	"${CMAKE_CURRENT_SOURCE_DIR}/IntermediateGenerator.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/IntermediateGenerator.h"
	"${CMAKE_CURRENT_SOURCE_DIR}/ExpressionResultType.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/ExpressionResultType.h"
)

set(sources ${sources} PARENT_SCOPE)
