
#pragma once

#include "lexer.h"
#include "ast.h"

struct parser {
    struct lexer *lexer;
    struct lexer_token *lookahead;
};

struct parser *parser_create(struct lexer *lexer);
void parser_destroy(struct parser *parser);

int parser_parse(struct parser *parser, struct ast_node **node);
