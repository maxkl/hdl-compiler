
use std::rc::Weak;

use failure::Fail;

#[derive(Debug, Fail)]
pub enum IntermediateError {
    #[fail(display = "attempt to add input signals after adding output signals, blocks or statements")]
    NoMoreInputSignals(),

    #[fail(display = "attempt to add output signals after adding blocks or statements")]
    NoMoreOutputSignals(),

    #[fail(display = "attempt to add block after adding statements")]
    NoMoreBlocks(),

    #[fail(display = "size {} is invalid for {} statement", _1, _0)]
    StatementSizeInvalid(String, u16),
}

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

    pub fn allocate_input_signals(&mut self, count: u32) -> Result<u32, IntermediateError> {
        if self.output_signal_count > 0 || !self.blocks.is_empty() || !self.statements.is_empty() {
            return Err(IntermediateError::NoMoreInputSignals());
        }

        let base_signal_id = self.next_signal_id;
        self.input_signal_count += count;
        self.next_signal_id += count;

        Ok(base_signal_id)
    }

    pub fn allocate_output_signals(&mut self, count: u32) -> Result<u32, IntermediateError> {
        if !self.blocks.is_empty() || !self.statements.is_empty() {
            return Err(IntermediateError::NoMoreOutputSignals());
        }

        let base_signal_id = self.next_signal_id;
        self.output_signal_count += count;
        self.next_signal_id += count;

        Ok(base_signal_id)
    }

    pub fn add_block(&mut self, block_weak: Weak<IntermediateBlock>) -> Result<u32, IntermediateError> {
        if !self.statements.is_empty() {
            return Err(IntermediateError::NoMoreBlocks());
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
    pub fn new(op: IntermediateOp, size: u16) -> Result<IntermediateStatement, IntermediateError> {
        let (input_count, output_count): (usize, usize) = match op {
            IntermediateOp::Connect => {
                if size != 1 {
                    return Err(IntermediateError::StatementSizeInvalid("Connect".to_string(), size));
                }

                (1, 1)
            },
            IntermediateOp::Const0 | IntermediateOp::Const1 => (0, 1),
            IntermediateOp::AND | IntermediateOp::OR | IntermediateOp::XOR => (size as usize, 1),
            IntermediateOp::NOT => {
                if size != 1 {
                    return Err(IntermediateError::StatementSizeInvalid("NOT".to_string(), size));
                }

                (1, 1)
            },
            IntermediateOp::MUX => (size as usize + (1 << size as usize), 1),
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
