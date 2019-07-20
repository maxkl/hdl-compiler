
use std::rc::Rc;
use std::cell::RefCell;

use crate::symbol_table::SymbolTable;
use crate::expression_type::ExpressionType;

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
pub struct RootNode {
    pub blocks: Vec<Rc<RefCell<BlockNode>>>,

    pub symbol_table: Option<Rc<RefCell<SymbolTable>>>
}

#[derive(Debug)]
pub struct BlockNode {
    pub name: Box<IdentifierNode>,
    pub declarations: Vec<Box<DeclarationNode>>,
    pub behaviour_statements: Vec<Box<BehaviourStatementNode>>,

    pub symbol_table: Option<Rc<RefCell<SymbolTable>>>
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

#[derive(Debug)]
pub enum TypeSpecifierNode {
    In,
    Out,
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
}

#[derive(Debug, Copy, Clone)]
pub enum UnaryOp {
    NOT,
}

#[derive(Debug)]
pub enum ExpressionNode {
    Binary(BinaryOp, Box<ExpressionNode>, Box<ExpressionNode>),
    Unary(UnaryOp, Box<ExpressionNode>),
    Variable(Box<BehaviourIdentifierNode>),
    Const(Box<NumberNode>),
}
