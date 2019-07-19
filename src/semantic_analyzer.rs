
use std::rc::{Rc, Weak};
use std::cell::RefCell;
use std::borrow::Borrow;

use failure::Fail;

use crate::ast::*;
use crate::symbol_table::{SymbolTable, SymbolTableError};
use crate::symbol::{SymbolType, SymbolTypeSpecifier, Symbol};

#[derive(Debug, Fail)]
pub enum SemanticAnalyzerError {
    #[fail(display = "reference to undefined symbol `{}`", _0)]
    UndefinedSymbol(String),

    #[fail(display = "symbol `{}` has invalid type: type `{}` required", _0, _1)]
    InvalidType(String, String),

    #[fail(display = "signal(s) declared with invalid width of 0")]
    ZeroWidth(),

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

        // Nested scope needed so that `block_ref` is dropped before `block` is used later on to prevent RefCell borrow panics
        {
            let mut block_ref = RefCell::borrow_mut(block.borrow());

            {
                let mut symbol_table_ref = RefCell::borrow_mut(symbol_table.borrow());

                for declaration in &mut block_ref.declarations {
                    Self::analyze_declaration(declaration, &mut symbol_table_ref)?;
                }

//                Self::analyze_behaviour_statements()?;
            }

            block_ref.symbol_table = Some(symbol_table);
        }

        let typ = Rc::new(SymbolType {
            specifier: SymbolTypeSpecifier::Block(Rc::downgrade(block)),
            width: 0
        });

        let mut global_symbol_table_ref = RefCell::borrow_mut(global_symbol_table.borrow());
        global_symbol_table_ref.add_type(typ)?;

        Ok(())
    }

    fn analyze_declaration(declaration: &mut DeclarationNode, symbol_table: &mut SymbolTable) -> Result<(), SemanticAnalyzerError> {
        let symbol_type = Self::analyze_type(declaration.typ.as_mut(), symbol_table)?;

        for name in &declaration.names {
            Self::analyze_declaration_identifier(name, symbol_table, &symbol_type)?;
        }

        Ok(())
    }

    fn analyze_type(typ: &mut TypeNode, symbol_table: &mut SymbolTable) -> Result<SymbolType, SemanticAnalyzerError> {
        let type_specifier = match typ.specifier.as_ref() {
            TypeSpecifierNode::In => SymbolTypeSpecifier::In,
            TypeSpecifierNode::Out => SymbolTypeSpecifier::Out,
            TypeSpecifierNode::Block(name) => {
                let name = &name.value;

                let block_symbol = symbol_table.find_type_recursive(name);

                let block_symbol_type = match &block_symbol {
                    Some(block_symbol_type) => block_symbol_type,
                    None => return Err(SemanticAnalyzerError::UndefinedSymbol(name.to_string())),
                };

                let block = match &block_symbol_type.specifier {
                    SymbolTypeSpecifier::Block(block) => block,
                    _ => return Err(SemanticAnalyzerError::InvalidType(name.to_string(), "block".to_string())),
                };

                SymbolTypeSpecifier::Block(Weak::clone(block))
            },
        };

        let width = if let Some(width_node) = &typ.width {
            let width = width_node.value;

            if width == 0 {
                return Err(SemanticAnalyzerError::ZeroWidth());
            }

            width
        } else {
            1
        };

        Ok(SymbolType {
            specifier: type_specifier,
            width: width,
        })
    }

    fn analyze_declaration_identifier(name: &IdentifierNode, symbol_table: &mut SymbolTable, symbol_type: &SymbolType) -> Result<(), SemanticAnalyzerError> {
        let name = &name.value;

        let symbol = Rc::new(Symbol {
            name: name.to_string(),
            typ: symbol_type.clone(),
        });

        symbol_table.add(symbol)?;

        Ok(())
    }
}
