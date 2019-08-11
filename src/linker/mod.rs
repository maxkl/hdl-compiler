
use std::result;
use std::rc::Rc;

use derive_more::Display;

use crate::shared::intermediate::Intermediate;
use crate::shared::error;

type Result = result::Result<Intermediate, Error>;

#[derive(Debug, Display)]
pub enum ErrorKind {
    #[display(fmt = "duplicate definition of block '{}'", _0)]
    DuplicateBlock(String),
}

pub type Error = error::Error<ErrorKind>;

pub struct Linker {
    //
}

impl Linker {
    pub fn link(inputs: &Vec<Intermediate>) -> Result {
        if inputs.is_empty() {
            return Ok(Vec::new());
        }

        let mut output = Vec::new();

        for input in inputs {
            Self::link_two(&mut output, input)?;
        }

        Ok(output)
    }

    fn link_two(target: &mut Intermediate, source: &Intermediate) -> result::Result<(), Error> {
        for source_block in source {
            let source_name = &source_block.name;

            for target_block in target.iter() {
                if &target_block.name == source_name {
                    return Err(ErrorKind::DuplicateBlock(source_name.clone()).into());
                }
            }

            target.push(Rc::clone(source_block));
        }

        Ok(())
    }
}
