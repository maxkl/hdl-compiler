
#include "frontend.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "lexer.h"
#include "parser.h"
#include "ast.h"
#include "analyzer.h"
#include "intermediate_generator.h"

int frontend_compile(const char *filename, struct intermediate_file *intermediate_file) {
	int ret;

	FILE *f = fopen(filename, "rb");
	if (f == NULL) {
		fprintf(stderr, "error: failed to open %s: %s\n", filename, strerror(errno));
		return 1;
	}

	struct lexer *lexer = lexer_create(f, filename);
	if (lexer == NULL) {
		return 1;
	}

	struct parser *parser = parser_create(lexer);
	if (parser == NULL) {
		fprintf(stderr, "error: failed to initialize the parser\n");
		return 1;
	}

	struct ast_node *ast_root;

	ret = parser_parse(parser, &ast_root);
	if (ret) {
		return ret;
	}

	parser_destroy(parser);
	lexer_destroy(lexer);
	
	fclose(f);

	ret = analyzer_analyze(ast_root);
	if (ret) {
		return ret;
	}

	ret = intermediate_generator_generate(ast_root, intermediate_file);
	if (ret) {
		return ret;
	}

	ast_destroy_node(ast_root);

	return 0;
}
