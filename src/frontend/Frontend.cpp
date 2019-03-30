
#include "Frontend.h"

#include <cstring>

#include "Lexer.h"
#include "Parser.h"
#include "SemanticAnalyzer.h"
#include "IntermediateGenerator.h"
#include <shared/errors.h>

namespace HDLCompiler {

Intermediate::File Frontend::compile(const std::string &filename) {
	std::ifstream f(filename, std::ios::binary);
	if (!f) {
		throw CompilerError("Failed to open " + filename + ": " + std::strerror(errno));
	}

	Lexer lexer(f, filename);

	Parser parser(lexer);

	std::unique_ptr<AST::RootNode> ast = parser.parse();

	SemanticAnalyzer::analyze(*ast);

	return IntermediateGenerator::generate(*ast);
}

}
