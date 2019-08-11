
use std::{error, fmt};
use std::fmt::{Debug, Display, Formatter};

#[derive(Debug)]
pub struct Error<K: Debug + Display> {
    pub kind: K,
    source: Option<Box<dyn error::Error + 'static>>,
}

impl<K: Debug + Display> Display for Error<K> {
    fn fmt(&self, f: &mut Formatter) -> fmt::Result {
        write!(f, "{}", self.kind)
    }
}

impl<K: Debug + Display> error::Error for Error<K> {
    fn source(&self) -> Option<&(dyn error::Error + 'static)> {
        self.source
            .as_ref()
            .map(|source| source.as_ref())
    }
}

impl<K: Debug + Display> From<K> for Error<K> {
    fn from(kind: K) -> Self {
        Error::new(kind)
    }
}

impl<K: Debug + Display> Error<K> {
    pub fn new(kind: K) -> Error<K> {
        Error {
            kind,
            source: None,
        }
    }

    pub fn with_boxed_source(kind: K, source: Box<dyn error::Error + 'static>) -> Error<K> {
        Error {
            kind,
            source: Some(source),
        }
    }

    pub fn with_source<E: error::Error + 'static>(kind: K, source: E) -> Error<K> {
        Self::with_boxed_source(kind, Box::new(source))
    }
}
