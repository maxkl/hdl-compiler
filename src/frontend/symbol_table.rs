
use std::collections::HashMap;
use std::collections::hash_map::Entry;
use std::slice::{Iter, IterMut};

use derive_more::Display;

use super::symbol::Symbol;
use crate::shared::error;

#[derive(Debug, Display)]
pub enum ErrorKind {
    #[display(fmt = "duplicate declaration of symbol '{}'", _0)]
    SymbolExists(String),
}

pub type Error = error::Error<ErrorKind>;

#[derive(Debug)]
pub struct SymbolTable {
    symbols: Vec<Symbol>,
    symbols_map: HashMap<String, usize>,
}

impl SymbolTable {
    pub fn new() -> SymbolTable {
        SymbolTable {
            symbols: Vec::new(),
            symbols_map: HashMap::new(),
        }
    }

    pub fn add(&mut self, symbol: Symbol) -> Result<(), Error> {
        match self.symbols_map.entry(symbol.name.clone()) {
            Entry::Occupied(o) => Err(ErrorKind::SymbolExists(o.key().to_string()).into()),
            Entry::Vacant(v) => {
                self.symbols.push(symbol);

                v.insert(self.symbols.len() - 1);

                Ok(())
            }
        }
    }

    pub fn find(&self, name: &str) -> Option<&Symbol> {
        self.symbols_map.get(name)
            .map(|&index| self.symbols.get(index).unwrap())
    }

    pub fn find_mut(&mut self, name: &str) -> Option<&mut Symbol> {
        // Can't use .map() because "closure may outlive the current function"
        match self.symbols_map.get(name) {
            Some(&index) => Some(self.symbols.get_mut(index).unwrap()),
            None => None,
        }
    }

    pub fn iter(&self) -> Iter<Symbol> {
        self.symbols.iter()
    }

    pub fn iter_mut(&mut self) -> IterMut<Symbol> {
        self.symbols.iter_mut()
    }
}
