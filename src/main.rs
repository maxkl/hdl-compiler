
mod ext_char;
mod char_reader;
mod lexer;
mod parser;

use std::{io, env};
use std::fs::File;
use std::io::Read;

use failure::{Error, format_err};

use crate::lexer::Lexer;
use crate::parser::Parser;

/// Wrapper (around stdin or a file) that implements `Read`
struct Input<'a> {
    source: Box<Read + 'a>,
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

fn run(args: Vec<String>) -> Result<(), Error> {
    // These are declared here to prolong their lifetime to outside the if-clause
    let stdin;
    let f;

    // Read from stdin when no arguments are supplied or read from a file if one is supplied
    let source = if args.len() == 1 {
        stdin = io::stdin();
        Input::from_stdin(&stdin)
    } else if args.len() == 2 {
        f = File::open(&args[1])?;
        Input::from_file(&f)
    } else {
        return Err(format_err!("invalid number of arguments"));
    };

    let lexer = Lexer::new(source);

    let mut parser = Parser::new(lexer);

    parser.parse()?;

    Ok(())
}

fn main() {
    let args = env::args().collect();

    if let Err(err) = run(args) {
        eprintln!("{}", err);
    }
}
