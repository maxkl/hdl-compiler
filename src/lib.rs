
mod shared;
mod frontend;
mod backend;

use std::io;
use std::fs::File;
use std::io::Read;

use failure::{Error, format_err};

use crate::frontend::lexer::Lexer;
use crate::frontend::parser::Parser;
use crate::frontend::semantic_analyzer::SemanticAnalyzer;
use crate::frontend::intermediate_generator::IntermediateGenerator;
use crate::backend::logic_simulator::LogicSimulator;

/// Wrapper (around stdin or a file) that implements `Read`
struct Input<'a> {
    source: Box<dyn Read + 'a>,
}

impl<'a> Read for Input<'a> {
    fn read(&mut self, buf: &mut [u8]) -> io::Result<usize> {
        self.source.read(buf)
    }
}

impl<'a> Input<'a> {
    fn from_stdin(stdin: &'a io::Stdin) -> Input<'a> {
        Input {
            source: Box::new(stdin.lock()),
        }
    }

    fn from_file(f: &'a File) -> Input<'a> {
        Input {
            source: Box::new(f),
        }
    }
}

pub fn run(args: Vec<String>) -> Result<(), Error> {
    // These are declared here to prolong their lifetime to outside the if-clause
    let stdin;
    let f;

    // Read from stdin when no arguments are supplied or read from a file if one is supplied
    let source = if args.len() == 1 {
        stdin = io::stdin();
        Input::from_stdin(&stdin)
    } else if args.len() >= 2 {
        f = File::open(&args[1])?;
        Input::from_file(&f)
    } else {
        return Err(format_err!("invalid number of arguments"));
    };

    let lexer = Lexer::new(source);

    let mut parser = Parser::new(lexer)?;

    let mut root = parser.parse()?;

    SemanticAnalyzer::analyze(&mut root)?;

    println!("{:#?}", root);

    let intermediate = IntermediateGenerator::generate(&root)?;

    println!("{:#?}", intermediate);

    let backend_args= if args.len() > 2 {
        &args[2..]
    } else {
        &[]
    };

    LogicSimulator::run(None, &intermediate, backend_args)?;

    Ok(())
}
