
#include "parser.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <shared/helper/mem.h>

struct parser *parser_create(struct lexer *lexer) {
    struct parser *parser = xmalloc(sizeof(struct parser));
    parser->lexer = lexer;
    parser->lookahead = NULL;
    return parser;
}

void parser_destroy(struct parser *parser) {
    if (parser->lookahead != NULL) {
        lexer_destroy_token(parser->lookahead);
    }
    xfree(parser);
}

static int parser_read_lookahead(struct parser *parser) {
    struct lexer_token *token;
    if (lexer_read_next_token(parser->lexer, &token)) {
        return -1;
    }
    parser->lookahead = token;
    return 0;
}

static int parser_match(struct parser *parser, enum lexer_token_type type, struct lexer_token **token) {
    if (parser->lookahead->type == type) {
        if (token == NULL) {
            lexer_destroy_token(parser->lookahead);
        } else {
            *token = parser->lookahead;
        }
        parser->lookahead = NULL;
        return parser_read_lookahead(parser);
    } else {
        lexer_print_location(stderr, parser->lookahead->location);
        fprintf(stderr, ": Expected %s but got %s\n", lexer_token_type_name(type), lexer_token_type_name(parser->lookahead->type));
        return -1;
    }
}

/*
 * blocks                           -> block blocks | E
 * block                            -> 'block' identifier '{' declarations behaviour_statements '}'
 * declarations                     -> declaration declarations | E
 * declaration                      -> type declaration_identifier_list ';'
 * declaration_identifier_list      -> declaration_identifier declaration_identifier_list_rest
 * declaration_identifier_list_rest -> ',' declaration_identifier declaration_identifier_list_rest | E
 * declaration_identifier           -> identifier declaration_width
 * declaration_width                -> '[' number ']' | E
 * type                             -> type_in | type_out | type_block
 * type_in                          -> 'in'
 * type_out                         -> 'out'
 * type_block                       -> 'block' identifier
 * behaviour_statements             -> behaviour_statement behaviour_statements | E
 * behaviour_statement              -> behaviour_identifier '=' behaviour_identifier ';'
 * behaviour_identifier             -> dotted_identifier subscript
 * dotted_identifier                -> identifier dotted_identifier_rest
 * dotted_identifier_rest           -> '.' identifier dotted_identifier_rest | E
 * subscript                        -> '[' number subscript_range ']' | E
 * subscript_range                  -> ':' number | E
 */

static int parse_identifier(struct parser *parser, struct ast_node **node) {
    int ret;

    struct lexer_token *token;

    ret = parser_match(parser, TOKEN_IDENTIFIER, &token);
    if (ret) {
        return ret;
    }

    *node = ast_create_node(AST_IDENTIFIER);
    (*node)->data.identifier = xstrdup(token->value.identifier);

    lexer_destroy_token(token);

    return 0;
}

static int parse_number(struct parser *parser, struct ast_node **node) {
    int ret;

    struct lexer_token *token;

    ret = parser_match(parser, TOKEN_NUMBER, &token);
    if (ret) {
        return ret;
    }

    *node = ast_create_node(AST_NUMBER);
    (*node)->data.number = token->value.number;

    lexer_destroy_token(token);

    return 0;
}

static int parse_expr(struct parser *parser, struct ast_node **node);

static int parse_operand(struct parser *parser, struct ast_node **node) {
    int ret;

    struct ast_node *expr = NULL;
    struct ast_node *identifier = NULL;

    switch (parser->lookahead->type) {
        case TOKEN_PARENTHESIS_LEFT:
            ret = parser_match(parser, TOKEN_PARENTHESIS_LEFT, NULL);
            if (ret) {
                goto err;
            }

            ret = parse_expr(parser, &expr);
            if (ret) {
                goto err;
            }

            ret = parser_match(parser, TOKEN_PARENTHESIS_RIGHT, NULL);
            if (ret) {
                goto err;
            }

            *node = ast_create_node(AST_OPERAND);
            ast_add_child_node(*node, expr);

            break;
        default: {
            ret = parse_identifier(parser, &identifier);
            if (ret) {
                goto err;
            }

            *node = ast_create_node(AST_OPERAND);
            ast_add_child_node(*node, identifier);
        }
    }

    return 0;

err:
    ast_destroy_node(identifier);
    ast_destroy_node(expr);
    return ret;
}

static int parse_andexpr_rest(struct parser *parser, struct ast_node **node) {
    int ret;

    struct ast_node *operand = NULL;

    switch (parser->lookahead->type) {
        case TOKEN_AND:
            ret = parser_match(parser, TOKEN_AND, NULL);
            if (ret) {
                goto err;
            }

            ret = parse_operand(parser, &operand);
            if (ret) {
                goto err;
            }

            *node = ast_create_node(AST_ANDEXPR_REST);
            ast_add_child_node(*node, operand);

            break;
        default:
            printf("-> E\n");
            *node = NULL;
    }

    return 0;

err:
    ast_destroy_node(operand);
    return ret;
}

static int parse_andexpr(struct parser *parser, struct ast_node **node) {
    int ret;

    struct ast_node *operand = NULL;
    struct ast_node *andexpr_rest = NULL;

    ret = parse_operand(parser, &operand);
    if (ret) {
        goto err;
    }

    ret = parse_andexpr_rest(parser, &andexpr_rest);
    if (ret) {
        goto err;
    }

    *node = ast_create_node(AST_ANDEXPR);
    ast_add_child_node(*node, operand);
    ast_add_child_node(*node, andexpr_rest);

    return 0;

err:
    ast_destroy_node(andexpr_rest);
    ast_destroy_node(operand);
    return ret;
}

static int parse_expr_rest(struct parser *parser, struct ast_node **node) {
    int ret;

    struct ast_node *andexpr = NULL;

    switch (parser->lookahead->type) {
        case TOKEN_OR:
            ret = parser_match(parser, TOKEN_OR, NULL);
            if (ret) {
                goto err;
            }

            ret = parse_andexpr(parser, &andexpr);
            if (ret) {
                goto err;
            }

            *node = ast_create_node(AST_EXPR_REST);
            ast_add_child_node(*node, andexpr);

            break;
        default:
            *node = NULL;
    }

    return 0;

err:
    ast_destroy_node(andexpr);
    return ret;
}

static int parse_expr(struct parser *parser, struct ast_node **node) {
    int ret;

    struct ast_node *andexpr = NULL;
    struct ast_node *expr_rest = NULL;

    ret = parse_andexpr(parser, &andexpr);
    if (ret) {
        goto err;
    }

    ret = parse_expr_rest(parser, &expr_rest);
    if (ret) {
        goto err;
    }

    *node = ast_create_node(AST_EXPR);
    ast_add_child_node(*node, andexpr);
    ast_add_child_node(*node, expr_rest);

    return 0;

err:
    ast_destroy_node(expr_rest);
    ast_destroy_node(andexpr);
    return ret;
}

static int parse_subscript_range(struct parser *parser, struct ast_node **node) {
    int ret;

    switch (parser->lookahead->type) {
        case TOKEN_COLON: {
            ret = parser_match(parser, TOKEN_COLON, NULL);
            if (ret) {
                return ret;
            }

            struct ast_node *number;
            ret = parse_number(parser, &number);
            if (ret) {
                return ret;
            }

            *node = ast_create_node(AST_SUBSCRIPT_RANGE);
            ast_add_child_node(*node, number);

            return 0;
        }
        default:
            *node = NULL;
            return 0;
    }
}

static int parse_subscript(struct parser *parser, struct ast_node **node) {
    int ret;

    switch (parser->lookahead->type) {
        case TOKEN_BRACKET_LEFT: {
            ret = parser_match(parser, TOKEN_BRACKET_LEFT, NULL);
            if (ret) {
                return ret;
            }

            struct ast_node *number;
            ret = parse_number(parser, &number);
            if (ret) {
                return ret;
            }

            struct ast_node *subscript_range;
            ret = parse_subscript_range(parser, &subscript_range);
            if (ret) {
                ast_destroy_node(number);
                return ret;
            }

            ret = parser_match(parser, TOKEN_BRACKET_RIGHT, NULL);
            if (ret) {
                ast_destroy_node(subscript_range);
                ast_destroy_node(number);
                return ret;
            }

            *node = ast_create_node(AST_SUBSCRIPT);
            ast_add_child_node(*node, number);
            ast_add_child_node(*node, subscript_range);

            return 0;
        }
        default:
            *node = NULL;
            return 0;
    }
}

static int parse_dotted_identifier_rest(struct parser *parser, struct ast_node **node) {
    int ret;

    switch (parser->lookahead->type) {
        case TOKEN_DOT: {
            ret = parser_match(parser, TOKEN_DOT, NULL);
            if (ret) {
                return ret;
            }

            struct ast_node *identifier;
            ret = parse_identifier(parser, &identifier);
            if (ret) {
                return ret;
            }

            struct ast_node *dotted_identifier_rest;
            ret = parse_dotted_identifier_rest(parser, &dotted_identifier_rest);
            if (ret) {
                ast_destroy_node(identifier);
                return ret;
            }

            *node = ast_create_node(AST_DOTTED_IDENTIFIER_REST);
            ast_add_child_node(*node, identifier);
            ast_add_child_node(*node, dotted_identifier_rest);

            return 0;
        }
        default:
            *node = NULL;
            return 0;
    }

    return 0;
}

static int parse_dotted_identifier(struct parser *parser, struct ast_node **node) {
    int ret;

    struct ast_node *identifier;
    ret = parse_identifier(parser, &identifier);
    if (ret) {
        goto err;
    }

    struct ast_node *dotted_identifier_rest;
    ret = parse_dotted_identifier_rest(parser, &dotted_identifier_rest);
    if (ret) {
        goto err_after_identifier;
    }

    *node = ast_create_node(AST_DOTTED_IDENTIFIER);
    ast_add_child_node(*node, identifier);
    ast_add_child_node(*node, dotted_identifier_rest);

    return 0;

//err_after_dotted_identifier_rest:
    ast_destroy_node(dotted_identifier_rest);
err_after_identifier:
    ast_destroy_node(identifier);
err:
    return ret;
}

static int parse_behaviour_identifier(struct parser *parser, struct ast_node **node) {
    int ret;

    struct ast_node *dotted_identifier;
    ret = parse_dotted_identifier(parser, &dotted_identifier);
    if (ret) {
        goto err;
    }

    struct ast_node *subscript;
    ret = parse_subscript(parser, &subscript);
    if (ret) {
        goto err_after_dotted_identifier;
    }

    *node = ast_create_node(AST_BEHAVIOUR_IDENTIFIER);
    ast_add_child_node(*node, dotted_identifier);
    ast_add_child_node(*node, subscript);

    return 0;

//err_after_subscript:
    ast_destroy_node(subscript);
err_after_dotted_identifier:
    ast_destroy_node(dotted_identifier);
err:
    return ret;
}

static int parse_behaviour_statement(struct parser *parser, struct ast_node **node) {
    int ret;

    struct ast_node *behaviour_identifier_1;
    ret = parse_behaviour_identifier(parser, &behaviour_identifier_1);
    if (ret) {
        goto err;
    }

    ret = parser_match(parser, TOKEN_EQUALS, NULL);
    if (ret) {
        goto err_after_behaviour_identifier_1;
    }

    struct ast_node *behaviour_identifier_2;
    ret = parse_expr(parser, &behaviour_identifier_2);
    if (ret) {
        goto err_after_behaviour_identifier_1;
    }

    ret = parser_match(parser, TOKEN_SEMICOLON, NULL);
    if (ret) {
        goto err_after_behaviour_identifier_2;
    }

    *node = ast_create_node(AST_BEHAVIOUR_STATEMENT);
    ast_add_child_node(*node, behaviour_identifier_1);
    ast_add_child_node(*node, behaviour_identifier_2);

    return 0;

err_after_behaviour_identifier_2:
    ast_destroy_node(behaviour_identifier_2);
err_after_behaviour_identifier_1:
    ast_destroy_node(behaviour_identifier_1);
err:
    return ret;
}

static int parse_behaviour_statements(struct parser *parser, struct ast_node **node) {
    int ret;

    switch (parser->lookahead->type) {
        case TOKEN_IDENTIFIER: {
            struct ast_node *behaviour_statement;
            ret = parse_behaviour_statement(parser, &behaviour_statement);
            if (ret) {
                return ret;
            }

            struct ast_node *behaviour_statements;
            ret = parse_behaviour_statements(parser, &behaviour_statements);
            if (ret) {
                ast_destroy_node(behaviour_statement);
                return ret;
            }

            *node = ast_create_node(AST_BEHAVIOUR_STATEMENTS);
            ast_add_child_node(*node, behaviour_statement);
            ast_add_child_node(*node, behaviour_statements);

            return 0;
        }
        default:
            *node = NULL;
            return 0;
    }
}

static int parse_type(struct parser *parser, struct ast_node **node) {
    int ret;

    switch (parser->lookahead->type) {
        case TOKEN_KEYWORD_IN:
            ret = parser_match(parser, TOKEN_KEYWORD_IN, NULL);
            if (ret) {
                return ret;
            }

            *node = ast_create_node(AST_TYPE_IN);

            return 0;
        case TOKEN_KEYWORD_OUT:
            ret = parser_match(parser, TOKEN_KEYWORD_OUT, NULL);
            if (ret) {
                return ret;
            }

            *node = ast_create_node(AST_TYPE_OUT);

            return 0;
        case TOKEN_KEYWORD_BLOCK:
            ret = parser_match(parser, TOKEN_KEYWORD_BLOCK, NULL);
            if (ret) {
                return ret;
            }

            struct ast_node *identifier;
            ret = parse_identifier(parser, &identifier);
            if (ret) {
                return ret;
            }

            *node = ast_create_node(AST_TYPE_BLOCK);
            ast_add_child_node(*node, identifier);

            return 0;
        default:
            lexer_print_location(stderr, parser->lookahead->location);
            fprintf(stderr, ": Expected type\n");
            return -1;
    }
}

static int parse_declaration_width(struct parser *parser, struct ast_node **node) {
    int ret;

    switch (parser->lookahead->type) {
        case TOKEN_BRACKET_LEFT: {
            ret = parser_match(parser, TOKEN_BRACKET_LEFT, NULL);
            if (ret) {
                return ret;
            }

            struct ast_node *number;
            ret = parse_number(parser, &number);
            if (ret) {
                return ret;
            }

            ret = parser_match(parser, TOKEN_BRACKET_RIGHT, NULL);
            if (ret) {
                ast_destroy_node(number);
                return ret;
            }

            *node = ast_create_node(AST_DECLARATION_WIDTH);
            ast_add_child_node(*node, number);

            return 0;
        }
        default:
            *node = NULL;
            return 0;
    }
}

static int parse_declaration_identifier(struct parser *parser, struct ast_node **node) {
    int ret;

    struct ast_node *identifier;
    ret = parse_identifier(parser, &identifier);
    if (ret) {
        goto err;
    }

    struct ast_node *declaration_width;
    ret = parse_declaration_width(parser, &declaration_width);
    if (ret) {
        goto err_after_identifier;
    }

    *node = ast_create_node(AST_DECLARATION_IDENTIFIER);
    ast_add_child_node(*node, identifier);
    ast_add_child_node(*node, declaration_width);

    return 0;

//err_after_declaration_width:
    ast_destroy_node(declaration_width);
err_after_identifier:
    ast_destroy_node(identifier);
err:
    return ret;
}

static int parse_declaration_identifier_list_rest(struct parser *parser, struct ast_node **node) {
    int ret;

    switch (parser->lookahead->type) {
        case TOKEN_COMMA: {
            ret = parser_match(parser, TOKEN_COMMA, NULL);
            if (ret) {
                return ret;
            }

            struct ast_node *declaration_identifier;
            ret = parse_declaration_identifier(parser, &declaration_identifier);
            if (ret) {
                return ret;
            }

            struct ast_node *declaration_identifier_list_rest;
            ret = parse_declaration_identifier_list_rest(parser, &declaration_identifier_list_rest);
            if (ret) {
                ast_destroy_node(declaration_identifier);
                return ret;
            }

            *node = ast_create_node(AST_DECLARATION_IDENTIFIER_LIST_REST);
            ast_add_child_node(*node, declaration_identifier);
            ast_add_child_node(*node, declaration_identifier_list_rest);

            return 0;
        }
        default:
            *node = NULL;
            return 0;
    }
}

static int parse_declaration_identifier_list(struct parser *parser, struct ast_node **node) {
    int ret;

    struct ast_node *declaration_identifier;
    ret = parse_declaration_identifier(parser, &declaration_identifier);
    if (ret) {
        goto err;
    }

    struct ast_node *declaration_identifier_list_rest;
    ret = parse_declaration_identifier_list_rest(parser, &declaration_identifier_list_rest);
    if (ret) {
        goto err_after_declaration_identifier;
    }

    *node = ast_create_node(AST_DECLARATION_IDENTIFIER_LIST);
    ast_add_child_node(*node, declaration_identifier);
    ast_add_child_node(*node, declaration_identifier_list_rest);

    return 0;

//err_after_declaration_identifier_list_rest:
    ast_destroy_node(declaration_identifier_list_rest);
err_after_declaration_identifier:
    ast_destroy_node(declaration_identifier);
err:
    return ret;
}

static int parse_declaration(struct parser *parser, struct ast_node **node) {
    int ret;

    struct ast_node *type;
    ret = parse_type(parser, &type);
    if (ret) {
        goto err;
    }

    struct ast_node *declaration_identifier_list;
    ret = parse_declaration_identifier_list(parser, &declaration_identifier_list);
    if (ret) {
        goto err_after_type;
    }

    ret = parser_match(parser, TOKEN_SEMICOLON, NULL);
    if (ret) {
        goto err_after_declaration_identifier_list;
    }

    *node = ast_create_node(AST_DECLARATION);
    ast_add_child_node(*node, type);
    ast_add_child_node(*node, declaration_identifier_list);

    return 0;

err_after_declaration_identifier_list:
    ast_destroy_node(declaration_identifier_list);
err_after_type:
    ast_destroy_node(type);
err:
    return ret;
}

static int parse_declarations(struct parser *parser, struct ast_node **node) {
    int ret;

    switch (parser->lookahead->type) {
        case TOKEN_KEYWORD_IN:
        case TOKEN_KEYWORD_OUT:
        case TOKEN_KEYWORD_BLOCK: {
            struct ast_node *declaration;
            ret = parse_declaration(parser, &declaration);
            if (ret) {
                return ret;
            }

            struct ast_node *declarations;
            ret = parse_declarations(parser, &declarations);
            if (ret) {
                ast_destroy_node(declaration);
                return ret;
            }

            *node = ast_create_node(AST_DECLARATIONS);
            ast_add_child_node(*node, declaration);
            ast_add_child_node(*node, declarations);

            return 0;
        }
        default:
            *node = NULL;
            return 0;
    }
}

static int parse_block(struct parser *parser, struct ast_node **node) {
    int ret;

    ret = parser_match(parser, TOKEN_KEYWORD_BLOCK, NULL);
    if (ret) {
        goto err;
    }

    struct ast_node *identifier;
    ret = parse_identifier(parser, &identifier);
    if (ret) {
        goto err;
    }

    ret = parser_match(parser, TOKEN_BRACE_LEFT, NULL);
    if (ret) {
        goto err_after_identifier;
    }

    struct ast_node *declarations;
    ret = parse_declarations(parser, &declarations);
    if (ret) {
        goto err_after_identifier;
    }

    struct ast_node *behaviour_statements;
    ret = parse_behaviour_statements(parser, &behaviour_statements);
    if (ret) {
        goto err_after_declarations;
    }

    ret = parser_match(parser, TOKEN_BRACE_RIGHT, NULL);
    if (ret) {
        goto err_after_behaviour_statements;
    }

    *node = ast_create_node(AST_BLOCK);
    ast_add_child_node(*node, identifier);
    ast_add_child_node(*node, declarations);
    ast_add_child_node(*node, behaviour_statements);

    return 0;

err_after_behaviour_statements:
    ast_destroy_node(behaviour_statements);
err_after_declarations:
    ast_destroy_node(declarations);
err_after_identifier:
    ast_destroy_node(identifier);
err:
    return ret;
}

static int parse_blocks(struct parser *parser, struct ast_node **node) {
    int ret;

    switch (parser->lookahead->type) {
        case TOKEN_KEYWORD_BLOCK: {
            struct ast_node *block;
            ret = parse_block(parser, &block);
            if (ret) {
                return ret;
            }

            struct ast_node *blocks;
            ret = parse_blocks(parser, &blocks);
            if (ret) {
                ast_destroy_node(block);
                return ret;
            }

            *node = ast_create_node(AST_BLOCKS);
            ast_add_child_node(*node, block);
            ast_add_child_node(*node, blocks);

            return 0;
        }
        default:
            *node = NULL;
            return 0;
    }
}

int parser_parse(struct parser *parser, struct ast_node **root_out) {
    int ret;

    ret = parser_read_lookahead(parser);
    if (ret) {
        return ret;
    }

    struct ast_node *root;

    ret = parse_blocks(parser, &root);
    if (ret) {
        return ret;
    }

    if (parser->lookahead->type != TOKEN_EOF) {
        lexer_print_location(stderr, parser->lookahead->location);
        fprintf(stderr, ": Unexpected %s, expected block\n", lexer_token_type_name(parser->lookahead->type));
        ast_destroy_node(root);
        return -1;
    }

    *root_out = root;

    return 0;
}
