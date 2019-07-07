
use failure::{Fail, Error};

use crate::lexer::{ILexer, TokenData, LexerError};

#[derive(Debug, Fail)]
pub enum ParserError {
    #[fail(display = "{}", _0)]
    Lexer(#[cause] LexerError),
}

impl From<LexerError> for ParserError {
    fn from(err: LexerError) -> Self {
        ParserError::Lexer(err)
    }
}

pub struct Parser<L: ILexer> {
    lexer: L
}

impl<L: ILexer> Parser<L> {
    pub fn new(lexer: L) -> Parser<L> {
        Parser {
            lexer
        }
    }

    pub fn parse(&mut self) -> Result<(), Error> {
        let mut terminate = false;

        // Read tokens until the end of the file is reached
        while !terminate {
            let token =  self.lexer.get_token()?;

            match token.data {
                TokenData::Identifier(name) => print!("Identifier({})", name),
                TokenData::Number(value) => print!("Number({})", value),
                TokenData::String(s) => print!("String(\"{}\")", s),

                TokenData::VarKeyword => print!("VarKeyword"),
                TokenData::PrintKeyword => print!("PrintKeyword"),

                TokenData::Semicolon => print!("Semicolon"),
                TokenData::LeftParenthesis => print!("LeftParenthesis"),
                TokenData::RightParenthesis => print!("RightParenthesis"),
                TokenData::LeftBrace => print!("LeftBrace"),
                TokenData::RightBrace => print!("RightBrace"),
                TokenData::LeftBracket => print!("LeftBracket"),
                TokenData::RightBracket => print!("RightBracket"),
                TokenData::Assign => print!("Assign"),
                TokenData::Plus => print!("Plus"),
                TokenData::Minus => print!("Minus"),

                TokenData::EndOfFile => {
                    print!("EndOfFile");
                    terminate = true;
                }
            }

            println!(" at {}", token.location);
        }

        Ok(())
    }
}
