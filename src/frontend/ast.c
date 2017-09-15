
#include "ast.h"

#include <stdio.h>
#include <stdlib.h>

#include <shared/helper/mem.h>

static const char *ast_node_type_names[] = {
    "AST_NONE",

    "AST_IDENTIFIER",
    "AST_NUMBER",

    "AST_BLOCKS",
    "AST_BLOCK",
    "AST_DECLARATIONS",
    "AST_DECLARATION",
    "AST_TYPE_IN",
    "AST_TYPE_OUT",
    "AST_TYPE_BLOCK",
    "AST_IDENTIFIER_LIST",
    "AST_IDENTIFIER_LIST_REST",
    "AST_BEHAVIOUR_STATEMENTS",
    "AST_BEHAVIOUR_STATEMENT",
    "AST_DOTTED_IDENTIFIER",
    "AST_DOTTED_IDENTIFIER_REST"
};

struct ast_node *ast_create_node(enum ast_node_type type) {
    struct ast_node *node = xmalloc(sizeof(struct ast_node));
    node->type = type;
    node->child_count = 0;
    node->children = NULL;
    return node;
}

void ast_destroy_node(struct ast_node *node) {
    if (node != NULL) {
        for (size_t i = 0; i < node->child_count; i++) {
            ast_destroy_node(node->children[i]);
        }
        xfree(node->children);
        switch (node->type) {
            case AST_IDENTIFIER:
                xfree(node->data.identifier);
                break;
            default:
                break;
        }
        xfree(node);
    }
}

void ast_add_child_node(struct ast_node *parent, struct ast_node *child) {
    parent->child_count++;
    parent->children = xrealloc(parent->children, sizeof(struct ast_node *) * parent->child_count);
    parent->children[parent->child_count - 1] = child;
}

static void print_node(struct ast_node *node, unsigned level) {
    if (node == NULL) {
        printf("%*s(null)\n", level * 2, "");
    } else {
        printf("%*s%s(%i):", level * 2, "", ast_node_type_names[node->type], node->type);
        switch (node->type) {
            case AST_IDENTIFIER:
                printf(" \"%s\"", node->data.identifier);
                break;
            default:
                break;
        }
        printf("\n");
        for (size_t i = 0; i < node->child_count; i++) {
            print_node(node->children[i], level + 1);
        }
    }
}

void ast_print_node(struct ast_node *node) {
    print_node(node, 0);
}
