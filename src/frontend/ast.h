
#pragma once

#include <stdlib.h>
#include <stdint.h>

#include "expression_type.h"

enum ast_node_type {
    AST_NONE = 0,

    AST_IDENTIFIER,
    AST_NUMBER,

    AST_BLOCKS,
    AST_BLOCK,
    AST_DECLARATIONS,
    AST_DECLARATION,
    AST_IDENTIFIER_LIST,
    AST_TYPE,
    AST_TYPE_SPECIFIER_IN,
    AST_TYPE_SPECIFIER_OUT,
    AST_TYPE_SPECIFIER_BLOCK,
    AST_BEHAVIOUR_STATEMENTS,
    AST_BEHAVIOUR_STATEMENT,
    AST_BEHAVIOUR_IDENTIFIER,
    AST_SUBSCRIPT,

    AST_BINARY_EXPRESSION,
    AST_UNARY_EXPRESSION
};

enum ast_op {
    AST_OP_NONE = 0,
    AST_OP_AND,
    AST_OP_OR,
    AST_OP_XOR,
    AST_OP_NOT
};

struct ast_node {
    enum ast_node_type type;
    size_t child_count;
    struct ast_node **children;
    union {
        char *identifier;
        uint64_t number;
        enum ast_op op;
    } data;
    union {
        struct symbol_table *symbol_table;
        struct expression_type *expression_type;
    } semantic_data;
};

struct ast_node *ast_create_node(enum ast_node_type type);
void ast_destroy_node(struct ast_node *node);

void ast_add_child_node(struct ast_node *parent, struct ast_node *child);

void ast_print_node(struct ast_node *node);
