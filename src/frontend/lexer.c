
#define _POSIX_C_SOURCE 200809L

#include "lexer.h"

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

static const char *token_type_names[] = {
    "(none)",
    "end of file",
    "identifier",
    "number",
    "'.'",
    "','",
    "';'",
    "'='",
    "'{'",
    "'}'",
    "'in'",
    "'out'",
    "'block'"
};

const char *lexer_token_type_name(enum lexer_token_type type) {
    if (type > LEXER_TOKEN_TYPE_MAX) {
        return "UNKNOWN";
    } else {
        return token_type_names[type];
    }
}

struct lexer_location *lexer_create_location() {
    struct lexer_location *location = malloc(sizeof(struct lexer_location));
    location->filename = NULL;
    return location;
}

void lexer_destroy_location(struct lexer_location *location) {
    if (location->filename != NULL) {
        free(location->filename);
    }
    free(location);
}

void lexer_copy_location(struct lexer_location *target, struct lexer_location *source) {
    if (target->filename != NULL) {
        free(target->filename);
    }
    target->filename = strdup(source->filename);
    target->line_number = source->line_number;
    target->column = source->column;
}

void lexer_print_location(FILE *stream, struct lexer_location *location) {
    fprintf(stream, "%s:%lu:%lu", location->filename, location->line_number, location->column);
}

struct lexer *lexer_create(FILE *f, const char *filename) {
    struct lexer *lexer = malloc(sizeof(struct lexer));
    lexer->f = f;
    lexer->location = lexer_create_location();
    lexer->location->line_number = 1;
    lexer->location->column = 0;
    lexer->location->filename = strdup(filename);
    lexer->last_location = lexer_create_location();
    return lexer;
}

void lexer_destroy(struct lexer *lexer) {
    lexer_destroy_location(lexer->location);
    lexer_destroy_location(lexer->last_location);
    free(lexer);
}

struct lexer_token *lexer_create_token() {
    struct lexer_token *token = malloc(sizeof(struct lexer_token));
    token->type = TOKEN_NONE;
    token->location = lexer_create_location();
    return token;
}

void lexer_destroy_token(struct lexer_token *token) {
    switch (token->type) {
        case TOKEN_IDENTIFIER:
            free(token->value.identifier);
            break;
        default:
            break;
    }
    lexer_destroy_location(token->location);
    free(token);
}

int lexer_getc(struct lexer *lexer) {
    lexer->last_c = lexer->c;
    lexer_copy_location(lexer->last_location, lexer->location);
    int c = fgetc(lexer->f);
    lexer->c = c;
    if (c == '\n') {
        lexer->location->line_number++;
        lexer->location->column = 0;
    } else {
        lexer->location->column++;
    }
    return c;
}

void lexer_ungetc(struct lexer *lexer) {
    if (lexer->c != EOF) {
        ungetc(lexer->c, lexer->f);
        lexer->c = lexer->last_c;
        lexer_copy_location(lexer->location, lexer->last_location);
    }
}

int lexer_read_next_token(struct lexer *lexer, struct lexer_token **token_out) {
    struct lexer_token *token = lexer_create_token();

    int c;

    c = lexer_getc(lexer);

    while (isspace(c)) {
        c = lexer_getc(lexer);
    }

    lexer_copy_location(token->location, lexer->location);

    if (isdigit(c)) {
        token->type = TOKEN_NUMBER;

        uint64_t num = 0;

        do {
            num *= 10;
            num += c - '0';
            c = lexer_getc(lexer);
        } while (isdigit(c));

        lexer_ungetc(lexer);

        token->value.number = num;
    } else if (isalpha(c) || c == '_') {

        char *str = NULL;
        size_t len = 0;

        do {
            len++;
            str = realloc(str, len);
            str[len - 1] = c;
            c = lexer_getc(lexer);
        } while (isalpha(c) || isdigit(c) || c == '_');

        lexer_ungetc(lexer);

        str = realloc(str, len + 1);
        str[len] = '\0';

        if (strcmp(str, "in") == 0) {
            free(str);
            token->type = TOKEN_KEYWORD_IN;
        } else if (strcmp(str, "out") == 0) {
            free(str);
            token->type = TOKEN_KEYWORD_OUT;
        } else if (strcmp(str, "block") == 0) {
            free(str);
            token->type = TOKEN_KEYWORD_BLOCK;
        } else {
            token->type = TOKEN_IDENTIFIER;
            token->value.identifier = str;
        }
    } else if (c == '.') {
        token->type = TOKEN_DOT;
    } else if (c == ',') {
        token->type = TOKEN_COMMA;
    } else if (c == ';') {
        token->type = TOKEN_SEMICOLON;
    } else if (c == '=') {
        token->type = TOKEN_EQUALS;
    } else if (c == '{') {
        token->type = TOKEN_BRACE_LEFT;
    } else if (c == '}') {
        token->type = TOKEN_BRACE_RIGHT;
    } else if (c == EOF) {
        token->type = TOKEN_EOF;
    } else {
        lexer_print_location(stderr, lexer->location);
        fprintf(stderr, ": Invalid character '%c'\n", c);
        lexer_destroy_token(token);
        return -1;
    }

    *token_out = token;

    return 0;
}
