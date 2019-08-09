
pub mod ext_char;
pub mod char_reader;
pub mod lexer;
pub mod ast;
pub mod symbol;
pub mod symbol_table;
pub mod parser;
pub mod expression_type;
pub mod semantic_analyzer;
pub mod intermediate_generator;

use std::{io, result};
use std::io::Read;
use std::fs::File;

use failure::Fail;

use crate::frontend::lexer::Lexer;
use crate::frontend::parser::{Parser, ParserError};
use crate::frontend::semantic_analyzer::{SemanticAnalyzer, SemanticAnalyzerError};
use crate::frontend::intermediate_generator::{IntermediateGenerator, IntermediateGeneratorError};
use crate::shared::intermediate::Intermediate;

pub type Result = result::Result<Intermediate, FrontendError>;

#[derive(Debug, Fail)]
pub enum FrontendError {
    #[fail(display = "{}", _0)]
    Parser(#[cause] ParserError),

    #[fail(display = "{}", _0)]
    SemanticAnalyzer(#[cause] SemanticAnalyzerError),

    #[fail(display = "{}", _0)]
    IntermediateGenerator(#[cause] IntermediateGeneratorError),
}

impl From<ParserError> for FrontendError {
    fn from(err: ParserError) -> Self {
        FrontendError::Parser(err)
    }
}

impl From<SemanticAnalyzerError> for FrontendError {
    fn from(err: SemanticAnalyzerError) -> Self {
        FrontendError::SemanticAnalyzer(err)
    }
}

impl From<IntermediateGeneratorError> for FrontendError {
    fn from(err: IntermediateGeneratorError) -> Self {
        FrontendError::IntermediateGenerator(err)
    }
}

/// Wrapper (around stdin or a file) that implements `Read`
pub struct Input<'a> {
    source: Box<dyn Read + 'a>,
}

impl<'a> Read for Input<'a> {
    fn read(&mut self, buf: &mut [u8]) -> io::Result<usize> {
        self.source.read(buf)
    }
}

impl<'a> Input<'a> {
    pub fn from_stdin(stdin: &'a io::Stdin) -> Input<'a> {
        Input {
            source: Box::new(stdin.lock()),
        }
    }

    pub fn from_file(f: &'a File) -> Input<'a> {
        Input {
            source: Box::new(f),
        }
    }
}

pub fn compile(source: Input) -> Result {
    let lexer = Lexer::new(source);

    let mut parser = Parser::new(lexer)?;

    let mut root = parser.parse()?;

    SemanticAnalyzer::analyze(&mut root)?;

    let intermediate = IntermediateGenerator::generate(&root)?;

    Ok(intermediate)
}
