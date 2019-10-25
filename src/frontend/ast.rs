
use std::rc::{Rc, Weak};
use std::cell::RefCell;
use std::collections::HashMap;
use std::path::PathBuf;

use super::symbol_table::SymbolTable;
use super::expression_type::ExpressionType;
use crate::shared::intermediate::IntermediateBlock;
use crate::frontend::cache::Cache;

#[derive(Debug)]
pub struct IdentifierNode {
    pub value: String
}

#[derive(Debug)]
pub struct NumberNode {
    pub value: u64,
    pub width: Option<u64>
}

#[derive(Debug)]
pub struct StringNode {
    pub value: String
}

#[derive(Debug)]
pub struct RootNode {
    pub includes: Vec<Box<IncludeNode>>,
    pub blocks: Vec<Rc<RefCell<BlockNode>>>,
    pub blocks_map: HashMap<String, usize>,
}

impl RootNode {
    pub fn find_block<'a>(&'a self, name: &str, cache: &'a Cache) -> Option<&'a Rc<RefCell<BlockNode>>> {
        self.find_block_local(name)
            .or_else(|| {
                self.includes.iter()
                    .find_map(|include| {
                        let path = include.full_path.as_ref().unwrap();
                        let cache_entry = cache.get(path).unwrap();
                        cache_entry.ast.as_ref().unwrap().find_block_local(name)
                    })
            })
    }

    pub fn find_block_local(&self, name: &str) -> Option<&Rc<RefCell<BlockNode>>> {
        self.blocks_map.get(name)
            .map(|&index| self.blocks.get(index).unwrap())
    }
}

#[derive(Debug)]
pub struct IncludeNode {
    pub name: Box<StringNode>,
    pub full_path: Option<PathBuf>,
}

#[derive(Debug)]
pub struct BlockNode {
    pub name: Box<IdentifierNode>,
    pub is_sequential: bool,
    pub declarations: Vec<Box<DeclarationNode>>,
    pub behaviour_statements: Vec<Box<BehaviourStatementNode>>,

    pub symbol_table: Option<Rc<RefCell<SymbolTable>>>,

    pub intermediate_block: Option<Weak<IntermediateBlock>>,
}

#[derive(Debug)]
pub struct DeclarationNode {
    pub typ: Box<TypeNode>,
    pub names: Vec<Box<IdentifierNode>>
}

#[derive(Debug)]
pub struct TypeNode {
    pub specifier: Box<TypeSpecifierNode>,
    pub width: Option<Box<NumberNode>>
}

#[derive(Copy, Clone, Debug)]
pub enum EdgeType {
    Rising,
    Falling,
}

#[derive(Debug)]
pub enum TypeSpecifierNode {
    Clock(EdgeType),
    In,
    Out,
    Wire,
    Block(Box<IdentifierNode>)
}

#[derive(Debug)]
pub struct BehaviourStatementNode {
    pub target: Box<BehaviourIdentifierNode>,
    pub source: Box<ExpressionNode>,

    pub expression_type: Option<ExpressionType>,
}

#[derive(Debug)]
pub struct BehaviourIdentifierNode {
    pub name: Box<IdentifierNode>,
    pub property: Option<Box<IdentifierNode>>,
    pub subscript: Option<Box<SubscriptNode>>
}

#[derive(Debug)]
pub struct SubscriptNode {
    pub upper: Option<Box<NumberNode>>,
    pub lower: Box<NumberNode>,

    pub upper_index: Option<u64>,
    pub lower_index: Option<u64>,
}

#[derive(Debug, Copy, Clone)]
pub enum BinaryOp {
    AND,
    OR,
    XOR,
    Add,
    Concatenate,
}

#[derive(Debug, Copy, Clone)]
pub enum UnaryOp {
    NOT,
}

#[derive(Debug)]
pub enum ExpressionNodeData {
    Binary(BinaryOp, Box<ExpressionNode>, Box<ExpressionNode>),
    Unary(UnaryOp, Box<ExpressionNode>),
    Variable(Box<BehaviourIdentifierNode>),
    Const(Box<NumberNode>),
}

#[derive(Debug)]
pub struct ExpressionNode {
    pub data: ExpressionNodeData,
    pub typ: Option<ExpressionType>,
}
