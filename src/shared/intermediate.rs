
use std::rc::{Weak, Rc};

use derive_more::Display;
use crate::shared::error;

#[derive(Debug, Display)]
pub enum ErrorKind {
    #[display(fmt = "attempt to add input signals after adding output signals, blocks or statements")]
    NoMoreInputSignals,

    #[display(fmt = "attempt to add output signals after adding blocks or statements")]
    NoMoreOutputSignals,

    #[display(fmt = "attempt to add block after adding statements")]
    NoMoreBlocks,

    #[display(fmt = "size {} is invalid for {} statement", _1, _0)]
    StatementSizeInvalid(String, u16),

    #[display(fmt = "duplicate definition of block '{}'", _0)]
    DuplicateBlock(String),
}

pub type Error = error::Error<ErrorKind>;

pub type Intermediate = Vec<Rc<IntermediateBlock>>;

#[derive(Debug, Clone, Copy)]
pub enum IntermediateOp {
    Connect,

    Const0,
    Const1,

    AND,
    OR,
    XOR,

    NOT,

    MUX,

    Add,

    FlipFlop,
}

#[derive(Debug)]
pub struct IntermediateStatement {
    pub op: IntermediateOp,
    pub size: u16,
    pub input_signal_ids: Vec<u32>,
    pub output_signal_ids: Vec<u32>,
}

#[derive(Debug)]
pub struct IntermediateBlock {
    pub name: String,

    pub input_signal_count: u32,
    pub output_signal_count: u32,
    pub blocks: Vec<Weak<IntermediateBlock>>,
    pub statements: Vec<IntermediateStatement>,
    pub next_signal_id: u32,
}

impl IntermediateBlock {
    pub fn new(name: String) -> IntermediateBlock {
        IntermediateBlock {
            name,
            input_signal_count: 0,
            output_signal_count: 0,
            blocks: Vec::new(),
            statements: Vec::new(),
            next_signal_id: 0,
        }
    }

    pub fn allocate_signals(&mut self, count: u32) -> u32 {
        let base_signal_id = self.next_signal_id;
        self.next_signal_id += count;
        base_signal_id
    }

    pub fn allocate_input_signals(&mut self, count: u32) -> Result<u32, Error> {
        if self.output_signal_count > 0 || !self.blocks.is_empty() || !self.statements.is_empty() {
            return Err(ErrorKind::NoMoreInputSignals.into());
        }

        let base_signal_id = self.next_signal_id;
        self.input_signal_count += count;
        self.next_signal_id += count;

        Ok(base_signal_id)
    }

    pub fn allocate_output_signals(&mut self, count: u32) -> Result<u32, Error> {
        if !self.blocks.is_empty() || !self.statements.is_empty() {
            return Err(ErrorKind::NoMoreOutputSignals.into());
        }

        let base_signal_id = self.next_signal_id;
        self.output_signal_count += count;
        self.next_signal_id += count;

        Ok(base_signal_id)
    }

    pub fn add_block(&mut self, block_weak: Weak<IntermediateBlock>) -> Result<u32, Error> {
        if !self.statements.is_empty() {
            return Err(ErrorKind::NoMoreBlocks.into());
        }

        let block = block_weak.upgrade().unwrap();

        let base_signal_id = self.next_signal_id;
        self.next_signal_id += block.input_signal_count + block.output_signal_count;

        self.blocks.push(block_weak);

        Ok(base_signal_id)
    }

    pub fn add_statement(&mut self, statement: IntermediateStatement) {
        self.statements.push(statement);
    }
}

impl IntermediateStatement {
    pub fn new(op: IntermediateOp, size: u16) -> Result<IntermediateStatement, Error> {
        let (input_count, output_count): (usize, usize) = match op {
            IntermediateOp::Connect => {
                if size != 1 {
                    return Err(ErrorKind::StatementSizeInvalid("Connect".to_string(), size).into());
                }

                (1, 1)
            },
            IntermediateOp::Const0 | IntermediateOp::Const1 => (0, 1),
            IntermediateOp::AND | IntermediateOp::OR | IntermediateOp::XOR => (size as usize, 1),
            IntermediateOp::NOT => {
                if size != 1 {
                    return Err(ErrorKind::StatementSizeInvalid("NOT".to_string(), size).into());
                }

                (1, 1)
            },
            IntermediateOp::MUX => (size as usize + (1 << size as usize), 1),
            IntermediateOp::Add => {
                if size != 2 && size != 3 {
                    return Err(ErrorKind::StatementSizeInvalid("Add".to_string(), size).into());
                }

                (size as usize, 2)
            },
            IntermediateOp::FlipFlop => {
                if size != 1 {
                    return Err(ErrorKind::StatementSizeInvalid("FlipFlop".to_string(), size).into());
                }

                (2, 1)
            }
        };

        Ok(IntermediateStatement {
            op,
            size,
            input_signal_ids: vec![0; input_count],
            output_signal_ids: vec![0; output_count],
        })
    }

    pub fn set_input(&mut self, index: u32, signal_id: u32) {
        self.input_signal_ids[index as usize] = signal_id;
    }

    pub fn set_output(&mut self, index: u32, signal_id: u32) {
        self.output_signal_ids[index as usize] = signal_id;
    }
}

pub fn merge(inputs: &Vec<Intermediate>) -> Result<Intermediate, Error> {
    if inputs.is_empty() {
        return Ok(Vec::new());
    }

    let mut output = Vec::new();

    for input in inputs {
        merge_two(&mut output, input)?;
    }

    Ok(output)
}

fn merge_two(target: &mut Intermediate, source: &Intermediate) -> Result<(), Error> {
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
