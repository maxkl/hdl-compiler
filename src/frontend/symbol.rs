
use std::rc::Weak;
use std::cell::RefCell;

use crate::frontend::ast::{BlockNode, EdgeType};

#[derive(Debug, Clone)]
pub enum SymbolTypeSpecifier {
    Clock(EdgeType),
    In,
    Out,
    Wire,
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

    pub signal_id: u32,
}
