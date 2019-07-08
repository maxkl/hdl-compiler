
use std::io::{BufReader, Read};
use std::{io, fmt};

use failure::Fail;

use crate::char_reader::CharReader;
use crate::ext_char::ExtChar;
use crate::ext_char::ExtChar::*;

#[derive(Debug, Fail)]
pub enum LexerError {
    #[fail(display = "unexpected character: '{}' at {}", _0, _1)]
    UnexpectedCharacter(char, Location),

    #[fail(display = "unexpected end of file at {}", _0)]
    UnexpectedEndOfFile(Location),

    #[fail(display = "number literal has no width specified after width separator at {}", _0)]
    NumberLiteralNoWidth(Location),

    #[fail(display = "number literal with width of 0 at {}", _0)]
    NumberLiteralWidthZero(Location),

    #[fail(display = "number literal with width greater than 64 at {}", _0)]
    NumberLiteralWidthTooBig(Location),

    #[fail(display = "number literal with value that doesn't fit into a register of specified width at {}", _0)]
    NumberLiteralValueTooBig(Location),

    #[fail(display = "{}", _0)]
    Io(#[cause] io::Error),
}

/// Trait for the Lexer to implement.
///
/// This simplifies testing the parser a lot, as it can be tested completely separate from the lexer
/// implementation
pub trait ILexer {
//    type Token = Token;
//    type Error = LexerError;

    fn get_token(&mut self) -> Result<Token, LexerError>;
}

/// Location of a token in the source code
#[derive(Copy, Clone, Debug, PartialEq)]
pub struct Location {
    pub line: usize,
    pub column: usize,
}

impl fmt::Display for Location {
    fn fmt(&self, f: &mut fmt::Formatter) -> Result<(), fmt::Error> {
        write!(f, "{}:{}", self.line, self.column)
    }
}

impl Location {
    fn new(line: usize, column: usize) -> Location {
        Location {
            line,
            column,
        }
    }
}

/// All the possible token types with associated data
#[derive(Debug, PartialEq)]
pub enum TokenData {
    EndOfFile,

    Identifier(String),
    Number { value: u64, width: Option<u64> },

    Dot,
    Comma,
    Semicolon,
    Colon,
    Equals,

    AND,
    OR,
    XOR,
    NOT,

    LeftBrace,
    RightBrace,
    LeftBracket,
    RightBracket,
    LeftParenthesis,
    RightParenthesis,

    InKeyword,
    OutKeyword,
    BlockKeyword,
}

/// Tokens of this type are emitted by the lexer
#[derive(Debug, PartialEq)]
pub struct Token {
    pub data: TokenData,
    pub location: Location,
}

impl Token {
    /// Construct a new token
    fn new(data: TokenData, location: Location) -> Token {
        Token {
            data,
            location,
        }
    }
}

/// Lexer implementation
pub struct Lexer<R: Read> {
    /// Buffered source
    reader: CharReader<BufReader<R>>,

    /// Current location in the source code
    location: Location,

    /// If this field is `Some(_)`, it is returned by `get_char()` before a new character is read
    /// from `reader`
    next_char: Option<ExtChar>,

    /// Character that was returned by the last call to `get_char()`.
    /// This is used by `unget_char()`.
    previous_char: Option<ExtChar>,
    /// Location of `previous_char`
    previous_location: Location,
}

impl<R: Read> ILexer for Lexer<R> {
//    type Token = Token;
//    type Error = LexerError;

    /// Reads from the source stream until a token is complete and returns it
    fn get_token(&mut self) -> Result<Token, LexerError> {
        let mut c;

        // Skip whitespace
        loop {
            c = self.get_char()?;

            match c {
                Char(x) if !x.is_whitespace() => break,
                EOF => break,
                _ => {}
            }
        }

        let token_location = self.location.clone();

        match c {
            // Simple tokens
            Char('.') => Ok(Token::new(TokenData::Dot, token_location)),
            Char(',') => Ok(Token::new(TokenData::Comma, token_location)),
            Char(';') => Ok(Token::new(TokenData::Semicolon, token_location)),
            Char(':') => Ok(Token::new(TokenData::Colon, token_location)),
            Char('=') => Ok(Token::new(TokenData::Equals, token_location)),
            Char('&') => Ok(Token::new(TokenData::AND, token_location)),
            Char('|') => Ok(Token::new(TokenData::OR, token_location)),
            Char('^') => Ok(Token::new(TokenData::XOR, token_location)),
            Char('~') => Ok(Token::new(TokenData::NOT, token_location)),
            Char('{') => Ok(Token::new(TokenData::LeftBrace, token_location)),
            Char('}') => Ok(Token::new(TokenData::RightBrace, token_location)),
            Char('[') => Ok(Token::new(TokenData::LeftBracket, token_location)),
            Char(']') => Ok(Token::new(TokenData::RightBracket, token_location)),
            Char('(') => Ok(Token::new(TokenData::LeftParenthesis, token_location)),
            Char(')') => Ok(Token::new(TokenData::RightParenthesis, token_location)),
            // Tokens that can't be match'ed
            Char(x) => {
                if x.is_alphabetic() || x == '_' {
                    // Identifier & keywords

                    let mut s = x.to_string();

                    loop {
                        c = self.get_char()?;

                        match c {
                            Char(x) => {
                                if x.is_alphanumeric() || x == '_' {
                                    s.push(x);
                                } else {
                                    self.unget_char();
                                    break;
                                }
                            },
                            EOF => break,
                        }
                    }

                    match s.as_ref() {
                        "in" => Ok(Token::new(TokenData::InKeyword, token_location)),
                        "out" => Ok(Token::new(TokenData::OutKeyword, token_location)),
                        "block" => Ok(Token::new(TokenData::BlockKeyword, token_location)),
                        _ => Ok(Token::new(TokenData::Identifier(s), token_location))
                    }
                } else if x.is_numeric() {
                    // Number

                    let value = self.parse_number(&mut c)?.unwrap();

                    let width = if c == Char('#') {
                        c = self.get_char()?;

                        let width = match self.parse_number(&mut c)? {
                            Some(v) => v,
                            None => return Err(LexerError::NumberLiteralNoWidth(token_location)),
                        };

                        if width == 0 {
                            return Err(LexerError::NumberLiteralWidthZero(token_location));
                        }

                        if width > 64 {
                            return Err(LexerError::NumberLiteralWidthTooBig(token_location));
                        }

                        if value > (1 << width) - 1 {
                            return Err(LexerError::NumberLiteralValueTooBig(token_location));
                        }

                        Some(width)
                    } else {
                        None
                    };

                    self.unget_char();

                    Ok(Token::new(TokenData::Number { value, width }, token_location))
                } else {
                    Err(LexerError::UnexpectedCharacter(x, token_location))
                }
            },
            // EndOfFile
            EOF => Ok(Token::new(TokenData::EndOfFile, token_location)),
        }
    }
}

impl<R: Read> Lexer<R> {
    /// Construct a new instance
    pub fn new(source: R) -> Lexer<R> {
        Lexer {
            reader: CharReader::new(BufReader::new(source)),
            location: Location::new(1, 0),
            next_char: None,
            previous_char: None,
            previous_location: Location::new(1, 0),
        }
    }

    /// Retrieve a single character from the source
    ///
    /// This method also keeps track of the current source code line and column number which is
    /// stored in `self.location`.
    /// When the end of the file is reached, this method returns `None`.
    fn get_char(&mut self) -> Result<ExtChar, LexerError> {
        let result = match self.next_char {
            Some(c) => {
                self.next_char = None;
                Ok(c)
            },
            None => self.reader.read_char(),
        };

        if let Ok(c) = result {
            self.previous_location = self.location;
            self.previous_char = Some(c);

            if let Char(c) = c {
                if c == '\n' {
                    self.location.line += 1;
                    self.location.column = 0;
                } else {
                    self.location.column += 1;
                }
            }
        }

        result.map_err(|e| LexerError::Io(e))
    }

    /// Undo the last call to `get_char()`
    ///
    /// If this method is called twice in a row (without calling `get_char()` in between), it will
    /// panic.
    fn unget_char(&mut self) {
        if self.next_char.is_some() {
            panic!("unget_char() called twice in a row");
        }

        match self.previous_char {
            Some(c) => {
                self.previous_char = None;
                self.next_char = Some(c);
                self.location = self.previous_location;
            },
            None => panic!("unget_char() called but no char to unget"),
        }
    }

    fn parse_number(&mut self, c: &mut ExtChar) -> Result<Option<u64>, LexerError> {
        if c.is_eof() || !c.get_char().unwrap().is_numeric() {
            return Ok(None);
        }

        let mut value: u64 = 0;

        loop {
            let digit = c.get_char().unwrap().to_digit(10).unwrap() as u64;

            value *= 10;
            value += digit;

            *c = self.get_char()?;

            match c {
                Char(x) if !x.is_numeric() => break,
                EOF => break,
                _ => {},
            }
        }

        Ok(Some(value))
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    use std::io::Cursor;

    use matches::assert_matches;

    fn expect_tokens(source_text: &str, expected_tokens: &Vec<Token>) -> Result<(), LexerError> {
        let source = Cursor::new(source_text);

        let mut lexer = Lexer::new(source);

        let mut tokens = Vec::new();

        let mut terminate = false;

        while !terminate {
            let token = lexer.get_token()?;

            if token.data == TokenData::EndOfFile {
                terminate = true;
            }

            tokens.push(token);
        }

        assert_eq!(expected_tokens, &tokens);

        Ok(())
    }

    #[test]
    fn identifier_with_digits_and_underscores() {
        let source_text = "_the_answer_is_42";
        let expected_tokens = vec![
            Token::new(TokenData::Identifier("_the_answer_is_42".to_string()), Location::new(1, 1)),
            Token::new(TokenData::EndOfFile, Location::new(1, 17)),
        ];

        let result = expect_tokens(source_text, &expected_tokens);
        assert!(result.is_ok());
    }

    #[test]
    fn number_zeroes() {
        let source_text = "00000000000000000000000005";
        let expected_tokens = vec![
            Token::new(TokenData::Number { value: 5, width: None }, Location::new(1, 1)),
            Token::new(TokenData::EndOfFile, Location::new(1, 26)),
        ];

        let result = expect_tokens(source_text, &expected_tokens);
        assert!(result.is_ok());
    }

    #[test]
    fn number_big() {
        let source_text = "9223372036854775807";
        let expected_tokens = vec![
            Token::new(TokenData::Number { value: 9223372036854775807, width: None }, Location::new(1, 1)),
            Token::new(TokenData::EndOfFile, Location::new(1, 19)),
        ];

        let result = expect_tokens(source_text, &expected_tokens);
        assert!(result.is_ok());
    }

    #[test]
    fn number_width() {
        let source_text = "42#8";
        let expected_tokens = vec![
            Token::new(TokenData::Number { value: 42, width: Some(8) }, Location::new(1, 1)),
            Token::new(TokenData::EndOfFile, Location::new(1, 4)),
        ];

        let result = expect_tokens(source_text, &expected_tokens);
        assert!(result.is_ok());
    }

    #[test]
    fn number_no_width() {
        let source_text = "42#";

        let source = Cursor::new(source_text);

        let mut lexer = Lexer::new(source);

        let result = lexer.get_token();

        assert_matches!(result, Err(LexerError::NumberLiteralNoWidth(_)));

        if let Err(LexerError::NumberLiteralNoWidth(l)) = result {
            assert_eq!(Location::new(1, 1), l);
        }
    }

    #[test]
    fn number_zero_width() {
        let source_text = "42#0";

        let source = Cursor::new(source_text);

        let mut lexer = Lexer::new(source);

        let result = lexer.get_token();

        assert_matches!(result, Err(LexerError::NumberLiteralWidthZero(_)));

        if let Err(LexerError::NumberLiteralWidthZero(l)) = result {
            assert_eq!(Location::new(1, 1), l);
        }
    }

    #[test]
    fn number_width_too_big() {
        let source_text = "42#70";

        let source = Cursor::new(source_text);

        let mut lexer = Lexer::new(source);

        let result = lexer.get_token();

        assert_matches!(result, Err(LexerError::NumberLiteralWidthTooBig(_)));

        if let Err(LexerError::NumberLiteralWidthTooBig(l)) = result {
            assert_eq!(Location::new(1, 1), l);
        }
    }

    #[test]
    fn number_value_too_big() {
        let source_text = "256#8";

        let source = Cursor::new(source_text);

        let mut lexer = Lexer::new(source);

        let result = lexer.get_token();

        assert_matches!(result, Err(LexerError::NumberLiteralValueTooBig(_)));

        if let Err(LexerError::NumberLiteralValueTooBig(l)) = result {
            assert_eq!(Location::new(1, 1), l);
        }
    }

    #[test]
    #[should_panic(expected = "attempt to add with overflow")]
    fn number_overflow() {
        let source_text = "18446744073709551616";

        let source = Cursor::new(source_text);

        let mut lexer = Lexer::new(source);

        let result = lexer.get_token();

        // TODO: return error before overflow instead of panicking
//        assert_matches!(result, Err(LexerError::NumberLiteralOverflow(_)));
    }

    #[test]
    fn unexpected_character() {
        let source_text = "jane?doe";

        let source = Cursor::new(source_text);

        let mut lexer = Lexer::new(source);

        let result = lexer.get_token();

        assert_matches!(result, Ok(_));

        let result = lexer.get_token();

        assert_matches!(result, Err(LexerError::UnexpectedCharacter(_, _)));

        if let Err(LexerError::UnexpectedCharacter(c, l)) = result {
            assert_eq!('?', c);
            assert_eq!(Location::new(1, 5), l);
        }
    }

    #[test]
    fn all_token_types() {
        let source_text = "\
hello
42
.
,
;
:
=
&
|
^
~
{
}
[
]
(
)
in
out
block
";

        let expected_tokens = vec![
            Token::new(TokenData::Identifier("hello".to_string()), Location::new(1, 1)),
            Token::new(TokenData::Number { value: 42, width: None }, Location::new(2, 1)),
            Token::new(TokenData::Dot, Location::new(3, 1)),
            Token::new(TokenData::Comma, Location::new(4, 1)),
            Token::new(TokenData::Semicolon, Location::new(5, 1)),
            Token::new(TokenData::Colon, Location::new(6, 1)),
            Token::new(TokenData::Equals, Location::new(7, 1)),
            Token::new(TokenData::AND, Location::new(8, 1)),
            Token::new(TokenData::OR, Location::new(9, 1)),
            Token::new(TokenData::XOR, Location::new(10, 1)),
            Token::new(TokenData::NOT, Location::new(11, 1)),
            Token::new(TokenData::LeftBrace, Location::new(12, 1)),
            Token::new(TokenData::RightBrace, Location::new(13, 1)),
            Token::new(TokenData::LeftBracket, Location::new(14, 1)),
            Token::new(TokenData::RightBracket, Location::new(15, 1)),
            Token::new(TokenData::LeftParenthesis, Location::new(16, 1)),
            Token::new(TokenData::RightParenthesis, Location::new(17, 1)),
            Token::new(TokenData::InKeyword, Location::new(18, 1)),
            Token::new(TokenData::OutKeyword, Location::new(19, 1)),
            Token::new(TokenData::BlockKeyword, Location::new(20, 1)),
            Token::new(TokenData::EndOfFile, Location::new(21, 0)),
        ];

        let result = expect_tokens(source_text, &expected_tokens);
        assert!(result.is_ok());
    }

    #[test]
    fn simple_program() {
        let source_text = "\
block and {
    in a[8];
    in b[8];
    out q[8];

    out = a & b;
}";

        let expected_tokens = vec![
            Token::new(TokenData::BlockKeyword, Location::new(1, 1)),
            Token::new(TokenData::Identifier("and".to_string()), Location::new(1, 7)),
            Token::new(TokenData::LeftBrace, Location::new(1, 11)),
            Token::new(TokenData::InKeyword, Location::new(2, 5)),
            Token::new(TokenData::Identifier("a".to_string()), Location::new(2, 8)),
            Token::new(TokenData::LeftBracket, Location::new(2, 9)),
            Token::new(TokenData::Number { value: 8, width: None }, Location::new(2, 10)),
            Token::new(TokenData::RightBracket, Location::new(2, 11)),
            Token::new(TokenData::Semicolon, Location::new(2, 12)),
            Token::new(TokenData::InKeyword, Location::new(3, 5)),
            Token::new(TokenData::Identifier("b".to_string()), Location::new(3, 8)),
            Token::new(TokenData::LeftBracket, Location::new(3, 9)),
            Token::new(TokenData::Number { value: 8, width: None }, Location::new(3, 10)),
            Token::new(TokenData::RightBracket, Location::new(3, 11)),
            Token::new(TokenData::Semicolon, Location::new(3, 12)),
            Token::new(TokenData::OutKeyword, Location::new(4, 5)),
            Token::new(TokenData::Identifier("q".to_string()), Location::new(4, 9)),
            Token::new(TokenData::LeftBracket, Location::new(4, 10)),
            Token::new(TokenData::Number { value: 8, width: None }, Location::new(4, 11)),
            Token::new(TokenData::RightBracket, Location::new(4, 12)),
            Token::new(TokenData::Semicolon, Location::new(4, 13)),
            Token::new(TokenData::OutKeyword, Location::new(6, 5)),
            Token::new(TokenData::Equals, Location::new(6, 9)),
            Token::new(TokenData::Identifier("a".to_string()), Location::new(6, 11)),
            Token::new(TokenData::AND, Location::new(6, 13)),
            Token::new(TokenData::Identifier("b".to_string()), Location::new(6, 15)),
            Token::new(TokenData::Semicolon, Location::new(6, 16)),
            Token::new(TokenData::RightBrace, Location::new(7, 1)),
            Token::new(TokenData::EndOfFile, Location::new(7, 1)),
        ];

        let result = expect_tokens(source_text, &expected_tokens);
        assert!(result.is_ok());
    }
}
