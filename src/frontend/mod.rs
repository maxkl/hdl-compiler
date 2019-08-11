
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

use derive_more::Display;

use crate::frontend::lexer::Lexer;
use crate::frontend::parser::Parser;
use crate::frontend::semantic_analyzer::SemanticAnalyzer;
use crate::frontend::intermediate_generator::IntermediateGenerator;
use crate::shared::intermediate::Intermediate;
use crate::shared::error;

pub type Result = result::Result<Intermediate, Error>;

#[derive(Debug, Display)]
pub enum ErrorKind {
    #[display(fmt = "failed to parse input file")]
    Parser,

    #[display(fmt = "semantic analysis failed")]
    SemanticAnalyzer,

    #[display(fmt = "failed to generate intermediate code")]
    IntermediateGenerator,
}

type Error = error::Error<ErrorKind>;

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

    let mut parser = Parser::new(lexer)
        .map_err(|err| Error::with_source(ErrorKind::Parser, err))?;

    let mut root = parser.parse()
        .map_err(|err| Error::with_source(ErrorKind::Parser, err))?;

    SemanticAnalyzer::analyze(&mut root)
        .map_err(|err| Error::with_source(ErrorKind::SemanticAnalyzer, err))?;

    let intermediate = IntermediateGenerator::generate(&root)
        .map_err(|err| Error::with_source(ErrorKind::IntermediateGenerator, err))?;

    Ok(intermediate)
}
