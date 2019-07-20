
use std::rc::Weak;
use std::cell::RefCell;

use super::ast::BlockNode;

#[derive(Debug, Clone)]
pub enum SymbolTypeSpecifier {
    In,
    Out,
    Block(Weak<RefCell<BlockNode>>),
}

#[derive(Debug, Clone)]
pub struct SymbolType {
    pub specifier: SymbolTypeSpecifier,
    pub width: u64
}

#[derive(Debug)]
pub struct Symbol {
    pub name: String,
    pub typ: SymbolType,
}
