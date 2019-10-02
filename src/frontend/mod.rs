
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

use std::result;
use std::path::Path;
use std::fs::File;
use std::io::BufReader;

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
    #[display(fmt = "failed to open file {}", _0)]
    FileOpen(String),

    #[display(fmt = "failed to parse input file")]
    Parser,

    #[display(fmt = "semantic analysis failed")]
    SemanticAnalyzer,

    #[display(fmt = "failed to generate intermediate code")]
    IntermediateGenerator,
}

type Error = error::Error<ErrorKind>;

pub fn compile(path: &Path) -> Result {
    let f = File::open(path)
        .map_err(|err| Error::with_source(ErrorKind::FileOpen(path.to_str().unwrap().to_string()), err))?;
    let reader = BufReader::new(f);

    let lexer = Lexer::new(reader);

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
