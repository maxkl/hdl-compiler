
use std::rc::{Rc, Weak};
use std::cell::RefCell;
use std::collections::HashMap;
use std::collections::hash_map::Entry;

use failure::Fail;

use super::symbol::{Symbol, SymbolType, SymbolTypeSpecifier};

#[derive(Debug, Fail)]
pub enum SymbolTableError {
    #[fail(display = "duplicate declaration of symbol '{}'", _0)]
    SymbolExists(String),

    #[fail(display = "duplicate declaration of type '{}'", _0)]
    TypeExists(String),
}

#[derive(Debug)]
pub struct SymbolTable {
    parent: Option<Weak<RefCell<SymbolTable>>>,
    symbols: HashMap<String, Rc<Symbol>>,
    types: HashMap<String, Rc<SymbolType>>,
}

impl SymbolTable {
    pub fn new(parent: Option<Weak<RefCell<SymbolTable>>>) -> SymbolTable {
        SymbolTable {
            parent,
            symbols: HashMap::new(),
            types: HashMap::new()
        }
    }

    pub fn add(&mut self, symbol: Rc<Symbol>) -> Result<(), SymbolTableError> {
        match self.symbols.entry(symbol.name.clone()) {
            Entry::Occupied(o) => Err(SymbolTableError::SymbolExists(o.key().to_string())),
            Entry::Vacant(v) => {
                v.insert(symbol);

                Ok(())
            }
        }
    }

    pub fn add_type(&mut self, typ: Rc<SymbolType>) -> Result<(), SymbolTableError> {
        let name = match &typ.specifier {
            SymbolTypeSpecifier::Block(block) => block.upgrade().unwrap().borrow().name.value.clone(),
            specifier => panic!("type {:?} not supported by symbol table", specifier)
        };

        match self.types.entry(name) {
            Entry::Occupied(o) => Err(SymbolTableError::TypeExists(o.key().to_string())),
            Entry::Vacant(v) => {
                v.insert(typ);

                Ok(())
            }
        }
    }

    pub fn find(&self, name: &str) -> Option<&Rc<Symbol>> {
        self.symbols.get(name)
    }

    pub fn find_recursive(&self, name: &str) -> Option<Rc<Symbol>> {
        let symbol = self.find(name);

        if symbol.is_none() {
            if let Some(parent_weak) = &self.parent {
                let parent_refcell = parent_weak.upgrade().unwrap();
                let parent = parent_refcell.borrow();
                return parent.find_recursive(name);
            }
        }

        symbol.map(|rc| Rc::clone(rc))
    }

    pub fn find_type(&self, name: &str) -> Option<&Rc<SymbolType>> {
        self.types.get(name)
    }

    pub fn find_type_recursive(&self, name: &str) -> Option<Rc<SymbolType>> {
        let typ = self.find_type(name);

        if typ.is_none() {
            if let Some(parent_weak) = &self.parent {
                let parent_refcell = parent_weak.upgrade().unwrap();
                let parent = parent_refcell.borrow();
                return parent.find_type_recursive(name);
            }
        }

        typ.map(|rc| Rc::clone(rc))
    }
}
