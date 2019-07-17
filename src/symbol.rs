
use std::rc::Weak;
use std::cell::RefCell;

use crate::ast::BlockNode;

#[derive(Debug)]
pub enum SymbolTypeSpecifier {
    In,
    Out,
    Block(Weak<RefCell<BlockNode>>),
}

#[derive(Debug)]
pub struct SymbolType {
    pub specifier: SymbolTypeSpecifier,
    pub width: u64
}

#[derive(Debug)]
pub struct Symbol {
    pub name: String,
    pub typ: SymbolType,
}
