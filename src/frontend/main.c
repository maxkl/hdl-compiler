
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>

#include "parser.h"
#include "analyzer.h"
#include "intermediate_generator.h"
#include <shared/intermediate_file.h>

#if 0
#define TIME_START(name) clock_t start_time_ ## name = clock()
#define TIME_END(name) clock_t end_time_ ## name = clock(); fprintf(stderr, # name ": %f ms\n", (double) (end_time_ ## name - start_time_ ## name) / CLOCKS_PER_SEC * 1000.0)
#else
#define TIME_START(name)
#define TIME_END(name)
#endif

int main(int argc, char **argv) {
    int ret;

    if (argc != 3) {
        fprintf(stderr, "Usage: %s <source file> <target file>\n", argv[0]);
        return -1;
    }

    char *source_filename = argv[1];
    char *target_filename = argv[2];
    FILE *source_file = NULL, *target_file = NULL;

    struct lexer *lexer;

    if (strcmp(source_filename, "-") == 0) {
        lexer = lexer_create(stdin, "stdin");
    } else {
        source_file = fopen(source_filename, "r");
        if (source_file == NULL) {
            perror("Failed to open source file");
            return -1;
        }

        lexer = lexer_create(source_file, source_filename);
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

    TIME_START(parsing);

    ret = parser_parse(parser, &ast_root);
    if (ret) {
        return ret;
    }

    TIME_END(parsing);

    // We're done with the parser, the lexer and the source file
    parser_destroy(parser);
    lexer_destroy(lexer);
    if (source_file != NULL) {
        fclose(source_file);
    }

    //ast_print_node(ast_root);

    TIME_START(semantic_analysis);

    ret = analyzer_analyze(ast_root);
    if (ret) {
        return ret;
    }

    TIME_END(semantic_analysis);

    // if (strcmp(target_filename, "-") == 0) {
    //     lexer = lexer_create(target_file, "stdin");
    // } else {
    //     target_file = fopen(target_filename, "r");
    //     if (target_file == NULL) {
    //         perror("Failed to open source file");
    //         return -1;
    //     }

    //     lexer = lexer_create(target_file, target_filename);
    // }

    struct intermediate_block **blocks;
    size_t block_count;

    TIME_START(intermediate_generation);

    ret = intermediate_generator_generate(ast_root, &blocks, &block_count);
    if (ret) {
        return ret;
    }

    TIME_END(intermediate_generation);

    ast_destroy_node(ast_root);

    ret = intermediate_file_write(target_filename, blocks, block_count);
    if (ret) {
        return ret;
    }

    return 0;
}