
#include "parser.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

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
 * blocks               -> ( block )*
 * block                -> 'block' identifier '{' declarations behaviour_statements '}'
 * declarations         -> ( declaration )*
 * declaration          -> type identifier_list ';'
 * identifier_list      -> identifier ( ',' identifier )*
 * type                 -> type_specifier [ '[' number ']' ]
 * type_specifier       -> 'in' | 'out' | 'block' identifier
 * behaviour_statements -> ( behaviour_statement )*
 * behaviour_statement  -> behaviour_identifier '=' expression ';'
 * behaviour_identifier -> identifier [ '.' identifier ] [ subscript ]
 * subscript            -> '[' number [ ':' number ] ']'
 */

/*
 * Operators:
 *
 * Unary:
 *   Bitwise NOT: ~
 * Bitwise AND: &
 * Bitwise XOR: ^
 * Bitwise OR: |
 */

/*
 * expr -> bit_or_expr
 * bit_or_expr -> bit_xor_expr ( '|' bit_xor_expr )*
 * bit_xor_expr -> bit_and_expr ( '^' bit_and_expr )*
 * bit_and_expr -> unary_expr ( '&' unary_expr )*
 * unary_expr -> '~' unary_expr | primary_expr
 * primary_expr -> '(' expr ')' | behaviour_identifier | number
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

static int parse_expression(struct parser *parser, struct ast_node **node);
static int parse_behaviour_identifier(struct parser *parser, struct ast_node **node);

static int parse_primary_expression(struct parser *parser, struct ast_node **node) {
    int ret;

    struct ast_node *expression = NULL;
    struct ast_node *behaviour_identifier = NULL;
    struct ast_node *number = NULL;

    switch (parser->lookahead->type) {
        case TOKEN_PARENTHESIS_LEFT:
            ret = parser_match(parser, TOKEN_PARENTHESIS_LEFT, NULL);
            if (ret) {
                goto err;
            }

            ret = parse_expression(parser, &expression);
            if (ret) {
                goto err;
            }

            ret = parser_match(parser, TOKEN_PARENTHESIS_RIGHT, NULL);
            if (ret) {
                goto err;
            }

            *node = expression;

            break;
        case TOKEN_IDENTIFIER:
            ret = parse_behaviour_identifier(parser, &behaviour_identifier);
            if (ret) {
                goto err;
            }

            *node = behaviour_identifier;

            break;
        case TOKEN_NUMBER:
            ret = parse_number(parser, &number);
            if (ret) {
                goto err;
            }

            *node = number;

            break;
        default:
            lexer_print_location(stderr, parser->lookahead->location);
            fprintf(stderr, ": Expected '(', identifier or number\n");
            ret = -1;
            goto err;
    }

    return 0;

err:
    ast_destroy_node(expression);
    ast_destroy_node(behaviour_identifier);
    ast_destroy_node(number);
    return ret;
}

static int parse_unary_expression(struct parser *parser, struct ast_node **node) {
    int ret;

    struct ast_node *unary_expression = NULL;
    struct ast_node *primary_expression = NULL;

    switch (parser->lookahead->type) {
        case TOKEN_NOT:
            ret = parser_match(parser, TOKEN_NOT, NULL);
            if (ret) {
                goto err;
            }

            ret = parse_unary_expression(parser, &unary_expression);
            if (ret) {
                goto err;
            }

            *node = ast_create_node(AST_UNARY_EXPRESSION);
            (*node)->data.op = AST_OP_NOT;
            ast_add_child_node(*node, unary_expression);

            break;
        default:
            ret = parse_primary_expression(parser, &primary_expression);
            if (ret) {
                goto err;
            }

            *node = primary_expression;
    }

    return 0;

err:
    ast_destroy_node(unary_expression);
    ast_destroy_node(primary_expression);
    return ret;
}

static int parse_bitwise_and_expression(struct parser *parser, struct ast_node **node) {
    int ret;

    struct ast_node *left = NULL;
    struct ast_node *right = NULL;

    ret = parse_unary_expression(parser, &left);
    if (ret) {
        goto err;
    }

    while (parser->lookahead->type == TOKEN_AND) {
        ret = parser_match(parser, TOKEN_AND, NULL);
        if (ret) {
            goto err;
        }

        ret = parse_unary_expression(parser, &right);
        if (ret) {
            goto err;
        }

        struct ast_node *new_left = ast_create_node(AST_BINARY_EXPRESSION);
        new_left->data.op = AST_OP_AND;
        ast_add_child_node(new_left, left);
        ast_add_child_node(new_left, right);

        left = new_left;
        right = NULL;
    }

    *node = left;

    return 0;

err:
    ast_destroy_node(left);
    ast_destroy_node(right);
    return ret;
}

static int parse_bitwise_xor_expression(struct parser *parser, struct ast_node **node) {
    int ret;

    struct ast_node *left = NULL;
    struct ast_node *right = NULL;

    ret = parse_bitwise_and_expression(parser, &left);
    if (ret) {
        goto err;
    }

    while (parser->lookahead->type == TOKEN_XOR) {
        ret = parser_match(parser, TOKEN_XOR, NULL);
        if (ret) {
            goto err;
        }

        ret = parse_bitwise_and_expression(parser, &right);
        if (ret) {
            goto err;
        }

        struct ast_node *new_left = ast_create_node(AST_BINARY_EXPRESSION);
        new_left->data.op = AST_OP_XOR;
        ast_add_child_node(new_left, left);
        ast_add_child_node(new_left, right);

        left = new_left;
        right = NULL;
    }

    *node = left;

    return 0;

err:
    ast_destroy_node(left);
    ast_destroy_node(right);
    return ret;
}

static int parse_bitwise_or_expression(struct parser *parser, struct ast_node **node) {
    int ret;

    struct ast_node *left = NULL;
    struct ast_node *right = NULL;

    ret = parse_bitwise_xor_expression(parser, &left);
    if (ret) {
        goto err;
    }

    while (parser->lookahead->type == TOKEN_OR) {
        ret = parser_match(parser, TOKEN_OR, NULL);
        if (ret) {
            goto err;
        }

        ret = parse_bitwise_xor_expression(parser, &right);
        if (ret) {
            goto err;
        }

        struct ast_node *new_left = ast_create_node(AST_BINARY_EXPRESSION);
        new_left->data.op = AST_OP_OR;
        ast_add_child_node(new_left, left);
        ast_add_child_node(new_left, right);

        left = new_left;
        right = NULL;
    }

    *node = left;

    return 0;

err:
    ast_destroy_node(left);
    ast_destroy_node(right);
    return ret;
}

static int parse_expression(struct parser *parser, struct ast_node **node) {
    return parse_bitwise_or_expression(parser, node);
}

static int parse_subscript(struct parser *parser, struct ast_node **node) {
    int ret;

    struct ast_node *number_start = NULL;
    struct ast_node *number_end = NULL;

    ret = parser_match(parser, TOKEN_BRACKET_LEFT, NULL);
    if (ret) {
        goto err;
    }

    ret = parse_number(parser, &number_start);
    if (ret) {
        goto err;
    }

    // Optional end index
    if (parser->lookahead->type == TOKEN_COLON) {
        ret = parser_match(parser, TOKEN_COLON, NULL);
        if (ret) {
            goto err;
        }

        ret = parse_number(parser, &number_end);
        if (ret) {
            goto err;
        }
    }

    ret = parser_match(parser, TOKEN_BRACKET_RIGHT, NULL);
    if (ret) {
        goto err;
    }

    *node = ast_create_node(AST_SUBSCRIPT);
    ast_add_child_node(*node, number_start);
    ast_add_child_node(*node, number_end);

    return 0;

err:
    ast_destroy_node(number_start);
    ast_destroy_node(number_end);
    return ret;
}

static int parse_behaviour_identifier(struct parser *parser, struct ast_node **node) {
    int ret;

    struct ast_node *identifier = NULL;
    struct ast_node *property_identifier = NULL;
    struct ast_node *subscript = NULL;

    ret = parse_identifier(parser, &identifier);
    if (ret) {
        goto err;
    }

    // Optional property access
    if (parser->lookahead->type == TOKEN_DOT) {
        ret = parser_match(parser, TOKEN_DOT, NULL);
        if (ret) {
            goto err;
        }

        ret = parse_identifier(parser, &property_identifier);
        if (ret) {
            goto err;
        }
    }

    // Optional subscript
    if (parser->lookahead->type == TOKEN_BRACKET_LEFT) {
        ret = parse_subscript(parser, &subscript);
        if (ret) {
            goto err;
        }
    }

    *node = ast_create_node(AST_BEHAVIOUR_IDENTIFIER);
    ast_add_child_node(*node, identifier);
    ast_add_child_node(*node, property_identifier);
    ast_add_child_node(*node, subscript);

    return 0;

err:
    ast_destroy_node(identifier);
    ast_destroy_node(property_identifier);
    ast_destroy_node(subscript);
    return ret;
}

static int parse_behaviour_statement(struct parser *parser, struct ast_node **node) {
    int ret;

    struct ast_node *behaviour_identifier = NULL;
    struct ast_node *expression = NULL;

    ret = parse_behaviour_identifier(parser, &behaviour_identifier);
    if (ret) {
        goto err;
    }

    ret = parser_match(parser, TOKEN_EQUALS, NULL);
    if (ret) {
        goto err;
    }

    ret = parse_expression(parser, &expression);
    if (ret) {
        goto err;
    }

    ret = parser_match(parser, TOKEN_SEMICOLON, NULL);
    if (ret) {
        goto err;
    }

    *node = ast_create_node(AST_BEHAVIOUR_STATEMENT);
    ast_add_child_node(*node, behaviour_identifier);
    ast_add_child_node(*node, expression);

    return 0;

err:
    ast_destroy_node(behaviour_identifier);
    ast_destroy_node(expression);
    return ret;
}

static int parse_behaviour_statements(struct parser *parser, struct ast_node **node) {
    int ret;

    struct ast_node *behaviour_statements = NULL;
    struct ast_node *behaviour_statement = NULL;

    while (parser->lookahead->type == TOKEN_IDENTIFIER) {
        ret = parse_behaviour_statement(parser, &behaviour_statement);
        if (ret) {
            goto err;
        }

        struct ast_node *new_behaviour_statements;

        if (behaviour_statements == NULL) {
            new_behaviour_statements = behaviour_statement;
        } else {
            new_behaviour_statements = ast_create_node(AST_BEHAVIOUR_STATEMENTS);
            ast_add_child_node(new_behaviour_statements, behaviour_statements);
            ast_add_child_node(new_behaviour_statements, behaviour_statement);
        }

        behaviour_statements = new_behaviour_statements;
        behaviour_statement = NULL;
    }

    *node = behaviour_statements;

    return 0;

err:
    ast_destroy_node(behaviour_statements);
    ast_destroy_node(behaviour_statement);
    return ret;
}

static int parse_identifier_list(struct parser *parser, struct ast_node **node) {
    int ret;

    struct ast_node *left = NULL;
    struct ast_node *right = NULL;

    ret = parse_identifier(parser, &left);
    if (ret) {
        goto err;
    }

    while (parser->lookahead->type == TOKEN_COMMA) {
        ret = parser_match(parser, TOKEN_COMMA, NULL);
        if (ret) {
            goto err;
        }

        ret = parse_identifier(parser, &right);
        if (ret) {
            goto err;
        }

        struct ast_node *new_left = ast_create_node(AST_IDENTIFIER_LIST);
        ast_add_child_node(new_left, left);
        ast_add_child_node(new_left, right);

        left = new_left;
        right = NULL;
    }

    *node = left;

    return 0;

err:
    ast_destroy_node(left);
    ast_destroy_node(right);
    return ret;
}

static int parse_type_specifier(struct parser *parser, struct ast_node **node) {
    int ret;

    struct ast_node *identifier = NULL;

    switch (parser->lookahead->type) {
        case TOKEN_KEYWORD_IN:
            ret = parser_match(parser, TOKEN_KEYWORD_IN, NULL);
            if (ret) {
                goto err;
            }

            *node = ast_create_node(AST_TYPE_SPECIFIER_IN);

            break;
        case TOKEN_KEYWORD_OUT:
            ret = parser_match(parser, TOKEN_KEYWORD_OUT, NULL);
            if (ret) {
                goto err;
            }

            *node = ast_create_node(AST_TYPE_SPECIFIER_OUT);

            break;
        case TOKEN_KEYWORD_BLOCK:
            ret = parser_match(parser, TOKEN_KEYWORD_BLOCK, NULL);
            if (ret) {
                goto err;
            }

            ret = parse_identifier(parser, &identifier);
            if (ret) {
                goto err;
            }

            *node = ast_create_node(AST_TYPE_SPECIFIER_BLOCK);
            ast_add_child_node(*node, identifier);

            break;
        default:
            lexer_print_location(stderr, parser->lookahead->location);
            fprintf(stderr, ": Expected type\n");
            ret = -1;
            goto err;
    }

    return 0;

err:
    ast_destroy_node(identifier);
    return ret;
}

static int parse_type(struct parser *parser, struct ast_node **node) {
    int ret;

    struct ast_node *type_specifier = NULL;
    struct ast_node *number = NULL;

    ret = parse_type_specifier(parser, &type_specifier);
    if (ret) {
        goto err;
    }

    // Optional type width
    if (parser->lookahead->type == TOKEN_BRACKET_LEFT) {
        ret = parser_match(parser, TOKEN_BRACKET_LEFT, NULL);
        if (ret) {
            goto err;
        }

        ret = parse_number(parser, &number);
        if (ret) {
            goto err;
        }

        ret = parser_match(parser, TOKEN_BRACKET_RIGHT, NULL);
        if (ret) {
            goto err;
        }
    }

    *node = ast_create_node(AST_TYPE);
    ast_add_child_node(*node, type_specifier);
    ast_add_child_node(*node, number);

    return 0;

err:
    ast_destroy_node(type_specifier);
    ast_destroy_node(number);
    return ret;
}

static int parse_declaration(struct parser *parser, struct ast_node **node) {
    int ret;

    struct ast_node *type = NULL;
    struct ast_node *identifier_list = NULL;

    ret = parse_type(parser, &type);
    if (ret) {
        goto err;
    }

    ret = parse_identifier_list(parser, &identifier_list);
    if (ret) {
        goto err;
    }

    ret = parser_match(parser, TOKEN_SEMICOLON, NULL);
    if (ret) {
        goto err;
    }

    *node = ast_create_node(AST_DECLARATION);
    ast_add_child_node(*node, type);
    ast_add_child_node(*node, identifier_list);

    return 0;

err:
    ast_destroy_node(type);
    ast_destroy_node(identifier_list);
    return ret;
}

static int parse_declarations(struct parser *parser, struct ast_node **node) {
    int ret;

    struct ast_node *declarations = NULL;
    struct ast_node *declaration = NULL;

    while (parser->lookahead->type == TOKEN_KEYWORD_IN
        || parser->lookahead->type == TOKEN_KEYWORD_OUT
        || parser->lookahead->type == TOKEN_KEYWORD_BLOCK
        ) {
        ret = parse_declaration(parser, &declaration);
        if (ret) {
            goto err;
        }

        struct ast_node *new_declarations;

        if (declarations == NULL) {
            new_declarations = declaration;
        } else {
            new_declarations = ast_create_node(AST_DECLARATIONS);
            ast_add_child_node(new_declarations, declarations);
            ast_add_child_node(new_declarations, declaration);
        }

        declarations = new_declarations;
        declaration = NULL;
    }

    *node = declarations;

    return 0;

err:
    ast_destroy_node(declarations);
    ast_destroy_node(declaration);
    return ret;
}

static int parse_block(struct parser *parser, struct ast_node **node) {
    int ret;

    struct ast_node *identifier = NULL;
    struct ast_node *declarations = NULL;
    struct ast_node *behaviour_statements = NULL;

    ret = parser_match(parser, TOKEN_KEYWORD_BLOCK, NULL);
    if (ret) {
        goto err;
    }

    ret = parse_identifier(parser, &identifier);
    if (ret) {
        goto err;
    }

    ret = parser_match(parser, TOKEN_BRACE_LEFT, NULL);
    if (ret) {
        goto err;
    }

    ret = parse_declarations(parser, &declarations);
    if (ret) {
        goto err;
    }

    ret = parse_behaviour_statements(parser, &behaviour_statements);
    if (ret) {
        goto err;
    }

    ret = parser_match(parser, TOKEN_BRACE_RIGHT, NULL);
    if (ret) {
        goto err;
    }

    *node = ast_create_node(AST_BLOCK);
    ast_add_child_node(*node, identifier);
    ast_add_child_node(*node, declarations);
    ast_add_child_node(*node, behaviour_statements);

    return 0;

err:
    ast_destroy_node(identifier);
    ast_destroy_node(declarations);
    ast_destroy_node(behaviour_statements);
    return ret;
}

static int parse_blocks(struct parser *parser, struct ast_node **node) {
    int ret;

    struct ast_node *blocks = NULL;
    struct ast_node *block = NULL;

    while (parser->lookahead->type == TOKEN_KEYWORD_BLOCK) {
        ret = parse_block(parser, &block);
        if (ret) {
            goto err;
        }

        struct ast_node *new_blocks;

        if (blocks == NULL) {
            new_blocks = block;
        } else {
            new_blocks = ast_create_node(AST_BLOCKS);
            ast_add_child_node(new_blocks, blocks);
            ast_add_child_node(new_blocks, block);
        }

        blocks = new_blocks;
        block = NULL;
    }

    *node = blocks;

    return 0;

err:
    ast_destroy_node(blocks);
    ast_destroy_node(block);
    return ret;
}

int parser_parse(struct parser *parser, struct ast_node **root_out) {
    int ret;

    clock_t start = clock();

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

    clock_t end = clock();
    double parse_time = (double) (end - start) / CLOCKS_PER_SEC;
    fprintf(stderr, "parsing took %f seconds\n", parse_time);

    return 0;
}
