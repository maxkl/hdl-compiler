
pub mod logic_simulator;

use std::result;

use derive_more::Display;

use crate::shared::error;

#[derive(Debug, Display)]
pub enum ErrorKind {
    #[display(fmt = "{}", _0)]
    Custom(String),
}

type Error = error::Error<ErrorKind>;

pub type Result = result::Result<(), Error>;
