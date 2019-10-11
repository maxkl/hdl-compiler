
use std::rc::Rc;
use std::cell::RefCell;
use std::collections::HashMap;

use derive_more::Display;

use crate::frontend::lexer;
use super::lexer::{ILexer, TokenKind, Token, Location, TokenData};
use super::ast::*;
use crate::shared::error;

#[derive(Debug, Display)]
pub enum ErrorKind {
    #[display(fmt = "expected {} but got {} at {}", _1, _2, _0)]
    UnexpectedToken(Location, TokenKind, TokenKind),
    #[display(fmt = "expected {} but got {} at {}", _1, _2, _0)]
    UnexpectedToken2(Location, String, TokenKind),
    #[display(fmt = "failed to read next token")]
    Lexer,
}

pub type Error = error::Error<ErrorKind>;

impl From<lexer::Error> for Error {
    fn from(err: lexer::Error) -> Self {
        Error::with_source(ErrorKind::Lexer, err)
    }
}

pub struct Parser<L: ILexer> {
    lexer: L,
    lookahead: Box<Token>
}

impl<L: ILexer> Parser<L> {
    pub fn new(mut lexer: L) -> Result<Parser<L>, Error> {
        // Read first lookahead token
        let lookahead = lexer.get_token()?;

        Ok(Parser {
            lexer,
            lookahead: Box::new(lookahead)
        })
    }

    fn match_token(&mut self, expected_kind: TokenKind) -> Result<Box<Token>, Error> {
        if self.lookahead.kind == expected_kind {
            // Read next lookahead token
            let mut token = Box::new(self.lexer.get_token()?);
            // Swap the new and the old lookahead tokens
            std::mem::swap(&mut self.lookahead, &mut token);
            // `token` now holds the old lookahead token which is the token we just matched against
            Ok(token)
        } else {
            Err(ErrorKind::UnexpectedToken(self.lookahead.location, expected_kind, self.lookahead.kind).into())
        }
    }

    fn parse_identifier(&mut self) -> Result<Box<IdentifierNode>, Error> {
        let identifier = self.match_token(TokenKind::Identifier)?;

        if let TokenData::Identifier(name) = identifier.data {
            Ok(Box::new(IdentifierNode {
                value: name
            }))
        } else {
            panic!("Token `kind` and `data` do not match up");
        }
    }

    fn parse_number(&mut self) -> Result<Box<NumberNode>, Error> {
        let number = self.match_token(TokenKind::Number)?;

        if let TokenData::Number { value, width } = number.data {
            Ok(Box::new(NumberNode {
                value,
                width
            }))
        } else {
            panic!("Token `kind` and `data` do not match up");
        }
    }

    fn parse_string(&mut self) -> Result<Box<StringNode>, Error> {
        let identifier = self.match_token(TokenKind::String)?;

        if let TokenData::String(value) = identifier.data {
            Ok(Box::new(StringNode {
                value
            }))
        } else {
            panic!("Token `kind` and `data` do not match up");
        }
    }

    /// Parser grammar (EBNF):
    ///
    /// ```ebnf
    /// root                 = includes blocks ;
    /// includes             = { include } ;
    /// include              = 'include', string ;
    /// blocks               = { block } ;
    /// block                = 'block', identifier, '{', declarations, behaviour_statements, '}' ;
    /// declarations         = { declaration } ;
    /// declaration          = type, identifier_list, ';' ;
    /// identifier_list      = identifier, { ',', identifier } ;
    /// type                 = type_specifier, [ '[', number, ']' ] ;
    /// type_specifier       = 'in' | 'out' | 'block', identifier ;
    /// behaviour_statements = { behaviour_statement } ;
    /// behaviour_statement  = behaviour_identifier, '=', expr, ';' ;
    /// behaviour_identifier = identifier, [ '.', identifier ], [ subscript ] ;
    /// subscript            = '[', number, [ ':', number ], ']' ;
    ///
    ///
    /// Operators (highest precedence level first):
    ///
    /// Unary:
    ///   Bitwise NOT: ~
    /// Bitwise AND: &
    /// Bitwise XOR: ^
    /// Bitwise OR: |
    ///
    ///
    /// expr         = bit_or_expr ;
    /// bit_or_expr  = bit_xor_expr, { '|', bit_xor_expr } ;
    /// bit_xor_expr = bit_and_expr, { '^', bit_and_expr } ;
    /// bit_and_expr = unary_expr, { '&', unary_expr } ;
    /// unary_expr   = '~', unary_expr | primary_expr ;
    /// primary_expr = '(', expr, ')' | behaviour_identifier | number ;
    /// ```

    pub fn parse(&mut self) -> Result<Box<RootNode>, Error> {
        let includes = self.parse_includes()?;

        let blocks = self.parse_blocks()?;

        // Make sure there are no tokens left over
        self.match_token(TokenKind::EndOfFile)?;

        Ok(Box::new(RootNode {
            includes,
            blocks,
            blocks_map: HashMap::new()
        }))
    }

    fn parse_includes(&mut self) -> Result<Vec<Box<IncludeNode>>, Error> {
        let mut includes = Vec::new();

        while self.lookahead.kind == TokenKind::IncludeKeyword {
            let include = self.parse_include()?;

            includes.push(include);
        }

        Ok(includes)
    }

    fn parse_include(&mut self) -> Result<Box<IncludeNode>, Error> {
        self.match_token(TokenKind::IncludeKeyword)?;

        let name = self.parse_string()?;

        Ok(Box::new(IncludeNode {
            name,
            full_path: None,
        }))
    }

    fn parse_blocks<'a>(&mut self) -> Result<Vec<Rc<RefCell<BlockNode>>>, Error> {
        let mut blocks = Vec::<Rc<RefCell<BlockNode>>>::new();

        while self.lookahead.kind == TokenKind::BlockKeyword {
            let block = self.parse_block()?;

            blocks.push(block);
        }

        Ok(blocks)
    }

    fn parse_block<'a>(&mut self) -> Result<Rc<RefCell<BlockNode>>, Error> {
        self.match_token(TokenKind::BlockKeyword)?;

        let name = self.parse_identifier()?;

        self.match_token(TokenKind::LeftBrace)?;

        let declarations = self.parse_declarations()?;

        let behaviour_statements = self.parse_behaviour_statements()?;

        self.match_token(TokenKind::RightBrace)?;

        Ok(Rc::new(RefCell::new(BlockNode {
            name,
            declarations,
            behaviour_statements,
            symbol_table: None,
            intermediate_block: None,
        })))
    }

    fn parse_declarations(&mut self) -> Result<Vec<Box<DeclarationNode>>, Error> {
        let mut declarations = Vec::<Box<DeclarationNode>>::new();

        while self.lookahead.kind == TokenKind::InKeyword
            || self.lookahead.kind == TokenKind::OutKeyword
            || self.lookahead.kind == TokenKind::BlockKeyword {

            let declaration = self.parse_declaration()?;

            declarations.push(declaration);
        }

        Ok(declarations)
    }

    fn parse_declaration(&mut self) -> Result<Box<DeclarationNode>, Error> {
        let typ = self.parse_type()?;

        let names = self.parse_identifier_list()?;

        self.match_token(TokenKind::Semicolon)?;

        Ok(Box::new(DeclarationNode {
            typ,
            names
        }))
    }

    fn parse_type(&mut self) -> Result<Box<TypeNode>, Error> {
        let specifier = self.parse_type_specifier()?;

        let width = if self.lookahead.kind == TokenKind::LeftBracket {
            self.match_token(TokenKind::LeftBracket)?;

            let width = self.parse_number()?;

            self.match_token(TokenKind::RightBracket)?;

            Some(width)
        } else {
            None
        };

        Ok(Box::new(TypeNode {
            specifier,
            width
        }))
    }

    fn parse_type_specifier(&mut self) -> Result<Box<TypeSpecifierNode>, Error> {
        match self.lookahead.kind {
            TokenKind::InKeyword => {
                self.match_token(TokenKind::InKeyword)?;

                Ok(Box::new(TypeSpecifierNode::In))
            },
            TokenKind::OutKeyword => {
                self.match_token(TokenKind::OutKeyword)?;

                Ok(Box::new(TypeSpecifierNode::Out))
            },
            TokenKind::BlockKeyword => {
                self.match_token(TokenKind::BlockKeyword)?;

                let name = self.parse_identifier()?;

                Ok(Box::new(TypeSpecifierNode::Block(name)))
            },
            kind => Err(ErrorKind::UnexpectedToken2(self.lookahead.location, "type".to_string(), kind).into())
        }
    }

    fn parse_identifier_list(&mut self) -> Result<Vec<Box<IdentifierNode>>, Error> {
        let mut identifiers = Vec::<Box<IdentifierNode>>::new();

        identifiers.push(self.parse_identifier()?);

        while self.lookahead.kind == TokenKind::Comma {
            self.match_token(TokenKind::Comma)?;

            identifiers.push(self.parse_identifier()?);
        }

        Ok(identifiers)
    }

    fn parse_behaviour_statements(&mut self) -> Result<Vec<Box<BehaviourStatementNode>>, Error> {
        let mut behaviour_statements = Vec::<Box<BehaviourStatementNode>>::new();

        while self.lookahead.kind == TokenKind::Identifier {
            let behaviour_statement = self.parse_behaviour_statement()?;

            behaviour_statements.push(behaviour_statement);
        }

        Ok(behaviour_statements)
    }

    fn parse_behaviour_statement(&mut self) -> Result<Box<BehaviourStatementNode>, Error> {
        let target = self.parse_behaviour_identifier()?;

        self.match_token(TokenKind::Equals)?;

        let source = self.parse_expression()?;

        self.match_token(TokenKind::Semicolon)?;

        Ok(Box::new(BehaviourStatementNode {
            target,
            source,
            expression_type: None
        }))
    }

    fn parse_behaviour_identifier(&mut self) -> Result<Box<BehaviourIdentifierNode>, Error> {
        let name = self.parse_identifier()?;

        let property = if self.lookahead.kind == TokenKind::Dot {
            self.match_token(TokenKind::Dot)?;

            Some(self.parse_identifier()?)
        } else {
            None
        };

        let subscript = if self.lookahead.kind == TokenKind::LeftBracket {
            Some(self.parse_subscript()?)
        } else {
            None
        };

        Ok(Box::new(BehaviourIdentifierNode {
            name,
            property,
            subscript
        }))
    }

    fn parse_subscript(&mut self) -> Result<Box<SubscriptNode>, Error> {
        self.match_token(TokenKind::LeftBracket)?;

        let first = self.parse_number()?;

        let upper;
        let lower;

        if self.lookahead.kind == TokenKind::Colon {
            self.match_token(TokenKind::Colon)?;

            lower = self.parse_number()?;
            upper = Some(first);
        } else {
            lower = first;
            upper = None;
        }

        self.match_token(TokenKind::RightBracket)?;

        Ok(Box::new(SubscriptNode {
            upper,
            lower,
            upper_index: None,
            lower_index: None,
        }))
    }

    fn parse_expression(&mut self) -> Result<Box<ExpressionNode>, Error> {
        self.parse_bitwise_or_expression()
    }

    fn parse_bitwise_or_expression(&mut self) -> Result<Box<ExpressionNode>, Error> {
        let mut left = self.parse_bitwise_xor_expression()?;

        while self.lookahead.kind == TokenKind::OR {
            self.match_token(TokenKind::OR)?;

            let right = self.parse_bitwise_xor_expression()?;

            left = Box::new(ExpressionNode {
                data: ExpressionNodeData::Binary(BinaryOp::OR, left, right),
                typ: None,
            })
        }

        Ok(left)
    }

    fn parse_bitwise_xor_expression(&mut self) -> Result<Box<ExpressionNode>, Error> {
        let mut left = self.parse_bitwise_and_expression()?;

        while self.lookahead.kind == TokenKind::XOR {
            self.match_token(TokenKind::XOR)?;

            let right = self.parse_bitwise_and_expression()?;

            left = Box::new(ExpressionNode {
                data: ExpressionNodeData::Binary(BinaryOp::XOR, left, right),
                typ: None,
            })
        }

        Ok(left)
    }

    fn parse_bitwise_and_expression(&mut self) -> Result<Box<ExpressionNode>, Error> {
        let mut left = self.parse_unary_expression()?;

        while self.lookahead.kind == TokenKind::AND {
            self.match_token(TokenKind::AND)?;

            let right = self.parse_unary_expression()?;

            left = Box::new(ExpressionNode {
                data: ExpressionNodeData::Binary(BinaryOp::AND, left, right),
                typ: None,
            })
        }

        Ok(left)
    }

    fn parse_unary_expression(&mut self) -> Result<Box<ExpressionNode>, Error> {
        match self.lookahead.kind {
            TokenKind::NOT => {
                self.match_token(TokenKind::NOT)?;

                let operand = self.parse_unary_expression()?;

                Ok(Box::new(ExpressionNode {
                    data: ExpressionNodeData::Unary(UnaryOp::NOT, operand),
                    typ: None,
                }))
            },
            _ => self.parse_primary_expression()
        }
    }

    fn parse_primary_expression(&mut self) -> Result<Box<ExpressionNode>, Error> {
        match self.lookahead.kind {
            TokenKind::LeftParenthesis => {
                self.match_token(TokenKind::LeftParenthesis)?;

                let subexpression = self.parse_expression()?;

                self.match_token(TokenKind::RightParenthesis)?;

                Ok(subexpression)
            },
            TokenKind::Identifier => {
                let identifier = self.parse_behaviour_identifier()?;

                Ok(Box::new(ExpressionNode {
                    data: ExpressionNodeData::Variable(identifier),
                    typ: None,
                }))
            },
            TokenKind::Number => {
                let number = self.parse_number()?;

                Ok(Box::new(ExpressionNode {
                    data: ExpressionNodeData::Const(number),
                    typ: None,
                }))
            },
            kind => Err(ErrorKind::UnexpectedToken2(self.lookahead.location, "expression".to_string(), kind).into())
        }
    }
}
