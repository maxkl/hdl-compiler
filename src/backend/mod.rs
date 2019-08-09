
pub mod logic_simulator;

use std::result;

use failure::Fail;

#[derive(Debug, Fail)]
pub enum BackendError {
    #[fail(display = "{}", _0)]
    Custom(String),
}

pub type Result = result::Result<(), BackendError>;
