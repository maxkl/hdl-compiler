
use std::io::{BufReader, Read};
use std::{io, fmt};

use failure::Fail;

use crate::char_reader::CharReader;

#[derive(Debug, Fail)]
pub enum LexerError {
    #[fail(display = "unexpected character: '{}' at {}", _0, _1)]
    UnexpectedCharacter(char, Location),

    #[fail(display = "unexpected end of file at {}", _0)]
    UnexpectedEndOfFile(Location),

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
    Number(u64),
    String(String),

    VarKeyword,
    PrintKeyword,

    Semicolon,
    LeftParenthesis,
    RightParenthesis,
    LeftBrace,
    RightBrace,
    LeftBracket,
    RightBracket,
    Assign,
    Minus,
    Plus,
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
    next_char: Option<char>,

    /// Character that was returned by the last call to `get_char()`.
    /// This is used by `unget_char()`.
    previous_char: Option<char>,
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
                Some(x) => {
                    if !x.is_whitespace() {
                        break;
                    }
                },
                None => break,
            }
        }

        let token_location = self.location.clone();

        match c {
            // String
            Some('"') => {
                let mut s = String::new();

                loop {
                    c = self.get_char()?;

                    match c {
                        Some(x) => {
                            if x != '"' {
                                s.push(x);
                            } else {
                                break;
                            }
                        },
                        None => {
                            return Err(LexerError::UnexpectedEndOfFile(token_location));
                        },
                    }
                }

                Ok(Token::new(TokenData::String(s), token_location))
            },
            // Simple tokens
            Some(';') => Ok(Token::new(TokenData::Semicolon, token_location)),
            Some('(') => Ok(Token::new(TokenData::LeftParenthesis, token_location)),
            Some(')') => Ok(Token::new(TokenData::RightParenthesis, token_location)),
            Some('{') => Ok(Token::new(TokenData::LeftBrace, token_location)),
            Some('}') => Ok(Token::new(TokenData::RightBrace, token_location)),
            Some('[') => Ok(Token::new(TokenData::LeftBracket, token_location)),
            Some(']') => Ok(Token::new(TokenData::RightBracket, token_location)),
            Some('=') => Ok(Token::new(TokenData::Assign, token_location)),
            Some('+') => Ok(Token::new(TokenData::Plus, token_location)),
            Some('-') => Ok(Token::new(TokenData::Minus, token_location)),
            // Tokens that can't be match'ed
            Some(x) => {
                if x.is_alphabetic() || x == '_' {
                    // Identifier & keywords

                    let mut s = x.to_string();

                    loop {
                        c = self.get_char()?;

                        match c {
                            Some(x) => {
                                if x.is_alphanumeric() || x == '_' {
                                    s.push(x);
                                } else {
                                    self.unget_char();
                                    break;
                                }
                            },
                            None => break,
                        }
                    }

                    match s.as_ref() {
                        "var" => Ok(Token::new(TokenData::VarKeyword, token_location)),
                        "print" => Ok(Token::new(TokenData::PrintKeyword, token_location)),
                        _ => Ok(Token::new(TokenData::Identifier(s), token_location))
                    }
                } else if x.is_numeric() {
                    // Number

                    let mut value: u64 = x.to_digit(10).unwrap() as u64;

                    loop {
                        c = self.get_char()?;

                        match c {
                            Some(x) => {
                                if x.is_numeric() {
                                    value *= 10;
                                    value += x.to_digit(10).unwrap() as u64;
                                } else {
                                    self.unget_char();
                                    break;
                                }
                            },
                            None => break,
                        }
                    }

                    Ok(Token::new(TokenData::Number(value), token_location))
                } else {
                    Err(LexerError::UnexpectedCharacter(x, token_location))
                }
            },
            // EndOfFile
            None => Ok(Token::new(TokenData::EndOfFile, token_location)),
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
    fn get_char(&mut self) -> Result<Option<char>, LexerError> {
        let result = match self.next_char {
            Some(c) => {
                self.next_char = None;
                Ok(Some(c))
            },
            None => self.reader.read_char(),
        };

        if let Ok(Some(c)) = result {
            self.previous_location = self.location;
            self.previous_char = Some(c);

            if c == '\n' {
                self.location.line += 1;
                self.location.column = 0;
            } else {
                self.location.column += 1;
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
    fn empty_string() {
        let source_text = "\"\"";
        let expected_tokens = vec![
            Token::new(TokenData::String("".to_string()), Location::new(1, 1)),
            Token::new(TokenData::EndOfFile, Location::new(1, 2)),
        ];

        let result = expect_tokens(source_text, &expected_tokens);
        assert!(result.is_ok());
    }

    #[test]
    fn unterminated_string() {
        let source_text = "\"hello world";

        let source = Cursor::new(source_text);

        let mut lexer = Lexer::new(source);

        let result = lexer.get_token();

        assert_matches!(result, Err(LexerError::UnexpectedEndOfFile(_)));
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
    fn keyword_in_string() {
        let source_text = "var \"print\"";
        let expected_tokens = vec![
            Token::new(TokenData::VarKeyword, Location::new(1, 1)),
            Token::new(TokenData::String("print".to_string()), Location::new(1, 5)),
            Token::new(TokenData::EndOfFile, Location::new(1, 11)),
        ];

        let result = expect_tokens(source_text, &expected_tokens);
        assert!(result.is_ok());
    }

    #[test]
    fn number_zeroes() {
        let source_text = "00000000000000000000000005";
        let expected_tokens = vec![
            Token::new(TokenData::Number(5), Location::new(1, 1)),
            Token::new(TokenData::EndOfFile, Location::new(1, 26)),
        ];

        let result = expect_tokens(source_text, &expected_tokens);
        assert!(result.is_ok());
    }

    #[test]
    fn number_big() {
        let source_text = "9223372036854775807";
        let expected_tokens = vec![
            Token::new(TokenData::Number(9223372036854775807), Location::new(1, 1)),
            Token::new(TokenData::EndOfFile, Location::new(1, 19)),
        ];

        let result = expect_tokens(source_text, &expected_tokens);
        assert!(result.is_ok());
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
        let source_text = "jane.doe";

        let source = Cursor::new(source_text);

        let mut lexer = Lexer::new(source);

        let result = lexer.get_token();

        assert_matches!(result, Ok(_));

        let result = lexer.get_token();

        assert_matches!(result, Err(LexerError::UnexpectedCharacter(_, _)));

        if let Err(LexerError::UnexpectedCharacter(c, l)) = result {
            assert_eq!('.', c);
            assert_eq!(Location::new(1, 5), l);
        }
    }

    #[test]
    fn all_token_types() {
        let source_text = "\
hello
42
\"hello world\"
var
print
;
(
)
{
}
[
]
=
-
+";

        let expected_tokens = vec![
            Token::new(TokenData::Identifier("hello".to_string()), Location::new(1, 1)),
            Token::new(TokenData::Number(42), Location::new(2, 1)),
            Token::new(TokenData::String("hello world".to_string()), Location::new(3, 1)),
            Token::new(TokenData::VarKeyword, Location::new(4, 1)),
            Token::new(TokenData::PrintKeyword, Location::new(5, 1)),
            Token::new(TokenData::Semicolon, Location::new(6, 1)),
            Token::new(TokenData::LeftParenthesis, Location::new(7, 1)),
            Token::new(TokenData::RightParenthesis, Location::new(8, 1)),
            Token::new(TokenData::LeftBrace, Location::new(9, 1)),
            Token::new(TokenData::RightBrace, Location::new(10, 1)),
            Token::new(TokenData::LeftBracket, Location::new(11, 1)),
            Token::new(TokenData::RightBracket, Location::new(12, 1)),
            Token::new(TokenData::Assign, Location::new(13, 1)),
            Token::new(TokenData::Minus, Location::new(14, 1)),
            Token::new(TokenData::Plus, Location::new(15, 1)),
            Token::new(TokenData::EndOfFile, Location::new(15, 1)),
        ];

        let result = expect_tokens(source_text, &expected_tokens);
        assert!(result.is_ok());
    }

    #[test]
    fn simple_program() {
        let source_text = "\
var x = 5;

var y = x + 3;

print \"Hello World!\";

print x;
print y;";

        let expected_tokens = vec![
            Token::new(TokenData::VarKeyword, Location::new(1, 1)),
            Token::new(TokenData::Identifier("x".to_string()), Location::new(1, 5)),
            Token::new(TokenData::Assign, Location::new(1, 7)),
            Token::new(TokenData::Number(5), Location::new(1, 9)),
            Token::new(TokenData::Semicolon, Location::new(1, 10)),
            Token::new(TokenData::VarKeyword, Location::new(3, 1)),
            Token::new(TokenData::Identifier("y".to_string()), Location::new(3, 5)),
            Token::new(TokenData::Assign, Location::new(3, 7)),
            Token::new(TokenData::Identifier("x".to_string()), Location::new(3, 9)),
            Token::new(TokenData::Plus, Location::new(3, 11)),
            Token::new(TokenData::Number(3), Location::new(3, 13)),
            Token::new(TokenData::Semicolon, Location::new(3, 14)),
            Token::new(TokenData::PrintKeyword, Location::new(5, 1)),
            Token::new(TokenData::String("Hello World!".to_string()), Location::new(5, 7)),
            Token::new(TokenData::Semicolon, Location::new(5, 21)),
            Token::new(TokenData::PrintKeyword, Location::new(7, 1)),
            Token::new(TokenData::Identifier("x".to_string()), Location::new(7, 7)),
            Token::new(TokenData::Semicolon, Location::new(7, 8)),
            Token::new(TokenData::PrintKeyword, Location::new(8, 1)),
            Token::new(TokenData::Identifier("y".to_string()), Location::new(8, 7)),
            Token::new(TokenData::Semicolon, Location::new(8, 8)),
            Token::new(TokenData::EndOfFile, Location::new(8, 8)),
        ];

        let result = expect_tokens(source_text, &expected_tokens);
        assert!(result.is_ok());
    }
}
