
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include "parser.h"

int analyze(struct ast_node *ast_root) {
    // TODO
    return 0;
}

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

    struct ast_node *ast_root;
    int parser_ret = parser_parse(parser, &ast_root);

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

    int analyze_ret = analyze(ast_root);

    ast_destroy_node(ast_root);

    if (analyze_ret) {
        return analyze_ret;
    }

    return 0;
}