
use failure::{Fail, Error};

use crate::lexer::{ILexer, TokenKind, LexerError, Token, Location, TokenData};
use crate::ast::*;

#[derive(Debug, Fail)]
pub enum ParserError {
    #[fail(display = "{}: expected {} but got {}", _0, _1, _2)]
    UnexpectedToken(Location, TokenKind, TokenKind),

    #[fail(display = "{}: {}", _0, _1)]
    Custom(Location, String),

    #[fail(display = "{}", _0)]
    Lexer(#[cause] LexerError),
}

impl From<LexerError> for ParserError {
    fn from(err: LexerError) -> Self {
        ParserError::Lexer(err)
    }
}

pub struct Parser<L: ILexer> {
    lexer: L,
    lookahead: Box<Token>
}

impl<L: ILexer> Parser<L> {
    pub fn new(mut lexer: L) -> Result<Parser<L>, ParserError> {
        // Read first lookahead token
        let lookahead = lexer.get_token()?;

        Ok(Parser {
            lexer,
            lookahead: Box::new(lookahead)
        })
    }

    fn match_token(&mut self, expected_kind: TokenKind) -> Result<Box<Token>, ParserError> {
        if self.lookahead.kind == expected_kind {
            // Read next lookahead token
            let mut token = Box::new(self.lexer.get_token()?);
            // Swap the new and the old lookahead tokens
            std::mem::swap(&mut self.lookahead, &mut token);
            // `token` now holds the old lookahead token which is the token we just matched against
            Ok(token)
        } else {
            Err(ParserError::UnexpectedToken(self.lookahead.location, expected_kind, self.lookahead.kind))
        }
    }

    fn parse_identifier(&mut self) -> Result<Box<IdentifierNode>, ParserError> {
        let identifier = self.match_token(TokenKind::Identifier)?;

        if let TokenData::Identifier(name) = identifier.data {
            Ok(Box::new(IdentifierNode {
                value: name
            }))
        } else {
            panic!("Token `kind` and `data` do not match up");
        }
    }

    fn parse_number(&mut self) -> Result<Box<NumberNode>, ParserError> {
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

    /// Parser grammar (EBNF):
    ///
    /// ```ebnf
    /// root                 = blocks ;
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

    pub fn parse(&mut self) -> Result<(), Error> {
        let blocks = self.parse_blocks()?;

        // Make sure there are no tokens left over
        self.match_token(TokenKind::EndOfFile)?;

        let root = RootNode {
            blocks
        };

        println!("{:#?}", root);

        Ok(())
    }

    fn parse_blocks(&mut self) -> Result<Vec<Box<BlockNode>>, ParserError> {
        let mut blocks = Vec::<Box<BlockNode>>::new();

        while self.lookahead.kind == TokenKind::BlockKeyword {
            let block = self.parse_block()?;

            blocks.push(block);
        }

        Ok(blocks)
    }

    fn parse_block(&mut self) -> Result<Box<BlockNode>, ParserError> {
        self.match_token(TokenKind::BlockKeyword)?;

        let name = self.parse_identifier()?;

        self.match_token(TokenKind::LeftBrace)?;

        let declarations = self.parse_declarations()?;

        let behaviour_statements = self.parse_behaviour_statements()?;

        self.match_token(TokenKind::RightBrace)?;

        Ok(Box::new(BlockNode {
            name,
            declarations,
            behaviour_statements,
        }))
    }

    fn parse_declarations(&mut self) -> Result<Vec<Box<DeclarationNode>>, ParserError> {
        let mut declarations = Vec::<Box<DeclarationNode>>::new();

        while self.lookahead.kind == TokenKind::InKeyword
            || self.lookahead.kind == TokenKind::OutKeyword
            || self.lookahead.kind == TokenKind::BlockKeyword {

            let declaration = self.parse_declaration()?;

            declarations.push(declaration);
        }

        Ok(declarations)
    }

    fn parse_declaration(&mut self) -> Result<Box<DeclarationNode>, ParserError> {
        let typ = self.parse_type()?;

        let names = self.parse_identifier_list()?;

        self.match_token(TokenKind::Semicolon)?;

        Ok(Box::new(DeclarationNode {
            typ,
            names
        }))
    }

    fn parse_type(&mut self) -> Result<Box<TypeNode>, ParserError> {
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

    fn parse_type_specifier(&mut self) -> Result<Box<TypeSpecifierNode>, ParserError> {
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
            kind => Err(ParserError::Custom(self.lookahead.location, format!("expected type but got {}", kind)))
        }
    }

    fn parse_identifier_list(&mut self) -> Result<Vec<Box<IdentifierNode>>, ParserError> {
        let mut identifiers = Vec::<Box<IdentifierNode>>::new();

        identifiers.push(self.parse_identifier()?);

        while self.lookahead.kind == TokenKind::Comma {
            self.match_token(TokenKind::Comma)?;

            identifiers.push(self.parse_identifier()?);
        }

        Ok(identifiers)
    }

    fn parse_behaviour_statements(&mut self) -> Result<Vec<Box<BehaviourStatementNode>>, ParserError> {
        Ok(vec![])
    }
}
