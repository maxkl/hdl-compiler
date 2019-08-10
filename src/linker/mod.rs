
use std::result;
use std::rc::Rc;

use failure::Fail;

use crate::shared::intermediate::Intermediate;

type Result = result::Result<Intermediate, LinkerError>;

#[derive(Debug, Fail)]
pub enum LinkerError {
    #[fail(display = "duplicate definition of block '{}'", _0)]
    DuplicateBlock(String),
}

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

    fn link_two(target: &mut Intermediate, source: &Intermediate) -> result::Result<(), LinkerError> {
        for source_block in source {
            let source_name = &source_block.name;

            for target_block in target.iter() {
                if &target_block.name == source_name {
                    return Err(LinkerError::DuplicateBlock(source_name.clone()));
                }
            }

            target.push(Rc::clone(source_block));
        }

        Ok(())
    }
}
