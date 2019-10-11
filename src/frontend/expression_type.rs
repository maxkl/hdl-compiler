
#[derive(Debug, PartialEq, Clone)]
pub struct AccessType {
    pub readable: bool,
    pub writable: bool,
}

impl AccessType {
    pub fn r() -> AccessType {
        AccessType {
            readable: true,
            writable: false,
        }
    }

    pub fn w() -> AccessType {
        AccessType {
            readable: false,
            writable: true,
        }
    }

    pub fn rw() -> AccessType {
        AccessType {
            readable: true,
            writable: true,
        }
    }
}

#[derive(Debug, Clone)]
pub struct ExpressionType {
    pub access_type: AccessType,
    pub width: u64,
}
