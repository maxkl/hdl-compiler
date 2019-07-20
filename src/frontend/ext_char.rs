
#[derive(Copy, Clone, PartialEq)]
pub enum ExtChar {
    Char(char),
    EOF,
}

impl ExtChar {
    pub fn is_eof(&self) -> bool {
        match self {
            ExtChar::Char(_) => false,
            ExtChar::EOF => true,
        }
    }

    pub fn get_char(&self) -> Option<char> {
        (*self).into()
    }
}

impl From<Option<char>> for ExtChar {
    fn from(opt: Option<char>) -> ExtChar {
        match opt {
            Some(c) => ExtChar::Char(c),
            None => ExtChar::EOF,
        }
    }
}

impl From<ExtChar> for Option<char> {
    fn from(c: ExtChar) -> Option<char> {
        match c {
            ExtChar::Char(c) => Some(c),
            ExtChar::EOF => None,
        }
    }
}
