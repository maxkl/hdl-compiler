
#pragma once

#include <stdio.h>
#include <stdint.h>

enum lexer_token_type {
    TOKEN_NONE = 0,

    TOKEN_EOF,

    TOKEN_IDENTIFIER,

    TOKEN_NUMBER,

    TOKEN_DOT,
    TOKEN_COMMA,
    TOKEN_SEMICOLON,
    TOKEN_COLON,
    TOKEN_EQUALS,
    TOKEN_AND,
    TOKEN_OR,
    TOKEN_XOR,
    TOKEN_NOT,

    TOKEN_BRACE_LEFT,
    TOKEN_BRACE_RIGHT,
    TOKEN_BRACKET_LEFT,
    TOKEN_BRACKET_RIGHT,
    TOKEN_PARENTHESIS_LEFT,
    TOKEN_PARENTHESIS_RIGHT,

    TOKEN_KEYWORD_IN,
    TOKEN_KEYWORD_OUT,
    TOKEN_KEYWORD_BLOCK,

    LEXER_TOKEN_TYPE_MAX
};

struct lexer_location {
    unsigned long line_number;
    unsigned long column;
    char *filename;
};

struct lexer {
    FILE* f;
    int c;
    struct lexer_location *location;
    int last_c;
    struct lexer_location *last_location;
};

struct lexer_token {
    enum lexer_token_type type;
    union {
        char *identifier;
        struct {
            uint64_t value;
            uint64_t width;
        } number;
    } value;
    struct lexer_location *location;
};

struct lexer_location *lexer_create_location();
void lexer_destroy_location(struct lexer_location *location);
void lexer_copy_location(struct lexer_location *target, struct lexer_location *source);
void lexer_print_location(FILE *stream, struct lexer_location *location);

struct lexer *lexer_create(FILE *f, const char *filename);
void lexer_destroy(struct lexer *lexer);

struct lexer_token *lexer_create_token();
void lexer_destroy_token(struct lexer_token *token);
void lexer_print_token(FILE *stream, struct lexer_token *token);

int lexer_read_next_token(struct lexer *lexer, struct lexer_token **token);

const char *lexer_token_type_name(enum lexer_token_type type);
