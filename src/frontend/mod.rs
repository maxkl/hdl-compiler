
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
pub mod cache;

use std::result;
use std::path::{Path, PathBuf};
use std::fs::File;
use std::io::BufReader;
use std::env;

use derive_more::Display;

use crate::frontend::lexer::Lexer;
use crate::frontend::parser::Parser;
use crate::frontend::semantic_analyzer::SemanticAnalyzer;
use crate::frontend::intermediate_generator::IntermediateGenerator;
use crate::shared::intermediate::Intermediate;
use crate::shared::{error, intermediate};
use crate::frontend::cache::{Cache, CacheEntry, CompilationState};

pub type Result = result::Result<Intermediate, Error>;

#[derive(Debug, Display)]
pub enum ErrorKind {
    #[display(fmt = "failed to open file {}", _0)]
    FileOpen(String),

    #[display(fmt = "include cycle detected: TODO")]
    CyclicInclude,

    #[display(fmt = "failed to parse input file")]
    Parser,

    #[display(fmt = "semantic analysis failed")]
    SemanticAnalyzer,

    #[display(fmt = "failed to generate intermediate code")]
    IntermediateGenerator,
}

type Error = error::Error<ErrorKind>;

fn make_absolute<P1: AsRef<Path>, P2: AsRef<Path>>(relative: P1, root: P2) -> PathBuf {
    let relative = relative.as_ref();
    let root = root.as_ref();

    if relative.is_absolute() {
        relative.to_path_buf()
    } else {
        root.join(relative)
    }
}

fn compile_subtree(path: &Path, is_root: bool, cache: &mut Cache) -> result::Result<(), Error> {
    assert!(path.is_absolute());

    // TODO: do something with is_root

    let directory = path.parent().unwrap();

    let f = File::open(path)
        .map_err(|err| Error::with_source(ErrorKind::FileOpen(path.to_str().unwrap().to_string()), err))?;
    let reader = BufReader::new(f);

    let lexer = Lexer::new(reader);

    let mut parser = Parser::new(lexer)
        .map_err(|err| Error::with_source(ErrorKind::Parser, err))?;

    let mut root = parser.parse()
        .map_err(|err| Error::with_source(ErrorKind::Parser, err))?;

    cache.add(CacheEntry {
        path: path.to_path_buf(),
        state: CompilationState::Parsed,
        ast: None,
        intermediate: None,
    });

    for include in &mut root.includes {
        let name = &include.name.value;
        let full_path = make_absolute(name, directory);

        include.full_path = Some(full_path.clone());

        if let Some(entry) = cache.get(&full_path) {
            if entry.state == CompilationState::Parsed {
                return Err(Error::new(ErrorKind::CyclicInclude));
            }
        } else {
            compile_subtree(&full_path, false, cache)?;
        }
    }

    SemanticAnalyzer::analyze(&mut root, cache)
        .map_err(|err| Error::with_source(ErrorKind::SemanticAnalyzer, err))?;

    let intermediate = IntermediateGenerator::generate(&root)
        .map_err(|err| Error::with_source(ErrorKind::IntermediateGenerator, err))?;

    let cache_entry = cache.get_mut(path).unwrap();
    cache_entry.ast = Some(root);
    cache_entry.intermediate = Some(intermediate);
    cache_entry.state = CompilationState::Compiled;

    Ok(())
}

pub fn compile(path: &Path) -> Result {
    let mut cache = Cache::new();

    let full_path = make_absolute(path, env::current_dir().unwrap());

    compile_subtree(&full_path, true, &mut cache)?;

    let all_intermediate = cache.collect_intermediate();

    intermediate::merge(&all_intermediate)
        .map_err(|err| Error::with_source(ErrorKind::IntermediateGenerator, err))
}
