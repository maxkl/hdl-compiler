
use failure::Fail;

#[derive(Debug, Fail)]
pub enum BackendError {
    #[fail(display = "{}", _0)]
    Custom(String),
}
