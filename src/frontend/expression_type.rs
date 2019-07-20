
#[derive(Debug, PartialEq, Clone)]
pub enum AccessType {
    Read,
    Write,
}

#[derive(Debug, PartialEq, Clone)]
pub struct ExpressionType {
    pub access_type: AccessType,
    pub width: u64,
}
