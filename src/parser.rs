
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

            println!("{:?} at {}", token.data, token.location);

            if let TokenData::EndOfFile = token.data {
                terminate = true;
            }
        }

        Ok(())
    }
}
