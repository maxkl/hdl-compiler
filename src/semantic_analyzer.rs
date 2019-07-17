
use std::rc::Rc;
use std::cell::RefCell;
use std::borrow::Borrow;

use failure::Fail;

use crate::ast::*;
use crate::symbol_table::{SymbolTable, SymbolTableError};
use crate::symbol::{SymbolType, SymbolTypeSpecifier};

#[derive(Debug, Fail)]
pub enum SemanticAnalyzerError {
    #[fail(display = "{}", _0)]
    Custom(String),

    #[fail(display = "{}", _0)]
    SymbolTable(#[cause] SymbolTableError),
}

impl From<SymbolTableError> for SemanticAnalyzerError {
    fn from(err: SymbolTableError) -> Self {
        SemanticAnalyzerError::SymbolTable(err)
    }
}

pub struct SemanticAnalyzer {
    //
}

impl SemanticAnalyzer {
    pub fn analyze(root: &mut RootNode) -> Result<(), SemanticAnalyzerError> {
        Self::analyze_root(root)
    }

    fn analyze_root(root: &mut RootNode) -> Result<(), SemanticAnalyzerError> {
        let global_symbol_table = Rc::new(RefCell::new(SymbolTable::new(None)));

        for block in &root.blocks {
            Self::analyze_block(block, &global_symbol_table)?;
        }

        root.symbol_table = Some(global_symbol_table);

        Ok(())
    }

    fn analyze_block(block: &Rc<RefCell<BlockNode>>, global_symbol_table: &Rc<RefCell<SymbolTable>>) -> Result<(), SemanticAnalyzerError> {
        let parent_weak = Rc::downgrade(global_symbol_table);

        let symbol_table = Rc::new(RefCell::new(SymbolTable::new(Some(parent_weak))));

        {
            let mut block_ref = RefCell::borrow_mut(block.borrow());
            block_ref.symbol_table = Some(symbol_table);

            // Nested scope needed so that `block_ref` is dropped before `block` is used later on to prevent RefCell borrow panics
        }

        let typ = Rc::new(SymbolType {
            specifier: SymbolTypeSpecifier::Block(Rc::downgrade(block)),
            width: 0
        });

        {
            let mut global_symbol_table_ref = RefCell::borrow_mut(global_symbol_table.borrow());
            global_symbol_table_ref.add_type(typ)?;
        }

//        Self::analyze_declarations()?;

//        Self::analyze_behaviour_statements()?;

        Ok(())
    }
}
