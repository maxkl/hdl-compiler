
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
    pub blocks: Vec<Box<BlockNode>>
}

#[derive(Debug)]
pub struct BlockNode {
    pub name: Box<IdentifierNode>,
    pub declarations: Vec<Box<DeclarationNode>>,
    pub behaviour_statements: Vec<Box<BehaviourStatementNode>>
}

#[derive(Debug)]
pub struct DeclarationNode {
    pub type_: Box<TypeNode>,
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
    pub source: Box<ExpressionNode>
}

#[derive(Debug)]
pub struct BehaviourIdentifierNode {
    pub name: Box<IdentifierNode>,
    pub property: Option<Box<IdentifierNode>>,
    pub subscript: Option<Box<SubscriptNode>>
}

#[derive(Debug)]
pub struct SubscriptNode {
    pub start: Box<NumberNode>,
    pub end: Option<Box<NumberNode>>
}

#[derive(Debug)]
pub enum BinaryOp {
    AND,
    OR,
    XOR,
}

#[derive(Debug)]
pub enum UnaryOp {
    NOT,
}

#[derive(Debug)]
pub enum ExpressionNode {
    Binary(BinaryOp, Box<ExpressionNode>, Box<ExpressionNode>),
    Unary(UnaryOp, Box<ExpressionNode>),
    Variable(Box<IdentifierNode>),
    Const(Box<NumberNode>),
}