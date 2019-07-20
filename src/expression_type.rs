
#[derive(Debug, PartialEq)]
pub enum AccessType {
    Read,
    Write,
}

#[derive(Debug, PartialEq)]
pub struct ExpressionType {
    pub access_type: AccessType,
    pub width: u64,
}
