
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>

#include "parser.h"
#include "analyzer.h"

#define TIME_START(name) clock_t start_time_ ## name = clock()
#define TIME_END(name) clock_t end_time_ ## name = clock(); fprintf(stderr, # name ": %f ms\n", (double) (end_time_ ## name - start_time_ ## name) / CLOCKS_PER_SEC * 1000.0)

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <source file>\n", argv[0]);
        return -1;
    }

    char *filename = argv[1];
    FILE *f;

    struct lexer *lexer;

    if (strcmp(filename, "-") == 0) {
        f = stdin;
        lexer = lexer_create(f, "stdin");
    } else {
        f = fopen(filename, "r");
        if (f == NULL) {
            perror("Failed to open source file");
            return -1;
        }

        lexer = lexer_create(f, filename);
    }

    if (lexer == NULL) {
        return -1;
    }

    struct parser *parser = parser_create(lexer);
    if (parser == NULL) {
        fprintf(stderr, "Failed to initialize the parser\n");
        return -1;
    }

    TIME_START(parsing);

    struct ast_node *ast_root;
    int parser_ret = parser_parse(parser, &ast_root);

    TIME_END(parsing);

    // We're done with the parser, the lexer and the source file
    parser_destroy(parser);
    lexer_destroy(lexer);
    if (f != stdin) {
        fclose(f);
    }

    // If parsing failed, we terminate
    if (parser_ret) {
        return parser_ret;
    }

    ast_print_node(ast_root);

    TIME_START(semantic_analysis);

    int analyze_ret = analyzer_analyze(ast_root);

    TIME_END(semantic_analysis);

    ast_destroy_node(ast_root);

    if (analyze_ret) {
        return analyze_ret;
    }

    return 0;
}