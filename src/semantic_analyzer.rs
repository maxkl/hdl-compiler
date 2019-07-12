
use failure::{Fail, Error};

use crate::ast::*;

#[derive(Debug, Fail)]
pub enum SemanticAnalyzerError {
    #[fail(display = "{}", _0)]
    Custom(String),
}

struct SemanticAnalyzer {
    //
}

impl SemanticAnalyzer {
    pub fn analyze(root: &Box<RootNode>) -> Result<(), SemanticAnalyzerError> {
        Ok(())
    }
}
