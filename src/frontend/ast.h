
#pragma once

#include <stdlib.h>

enum ast_node_type {
    AST_NONE = 0,

    AST_IDENTIFIER,
    AST_NUMBER,

    AST_BLOCKS,
    AST_BLOCK,
    AST_DECLARATIONS,
    AST_DECLARATION,
    AST_TYPE_IN,
    AST_TYPE_OUT,
    AST_TYPE_BLOCK,
    AST_IDENTIFIER_LIST,
    AST_IDENTIFIER_LIST_REST,
    AST_BEHAVIOUR_STATEMENTS,
    AST_BEHAVIOUR_STATEMENT,
    AST_DOTTED_IDENTIFIER,
    AST_DOTTED_IDENTIFIER_REST
};

struct ast_node {
    enum ast_node_type type;
    size_t child_count;
    struct ast_node **children;
    union {
        char *identifier;
    } data;
};

struct ast_node *ast_create_node(enum ast_node_type type);
void ast_destroy_node(struct ast_node *node);

void ast_add_child_node(struct ast_node *parent, struct ast_node *child);

void ast_print_node(struct ast_node *node);
