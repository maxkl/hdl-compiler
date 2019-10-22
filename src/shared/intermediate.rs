
use std::rc::{Weak, Rc};
use std::fmt;

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

impl fmt::Display for IntermediateStatement {
    fn fmt(&self, f: &mut fmt::Formatter) -> Result<(), fmt::Error> {
        write!(f, "{:?}(size: {}, inputs: ", self.op, self.size)?;
        for (i, id) in self.input_signal_ids.iter().enumerate() {
            if i > 0 {
                write!(f, " ")?;
            }
            write!(f, "{}", id)?;
        }
        write!(f, ", outputs: ")?;
        for (i, id) in self.output_signal_ids.iter().enumerate() {
            if i > 0 {
                write!(f, " ")?;
            }
            write!(f, "{}", id)?;
        }
        write!(f, ")")?;

        Ok(())
    }
}

#[derive(Debug)]
pub struct IntermediateBlock {
    pub name: String,

    pub input_signal_count: u32,
    pub output_signal_count: u32,
    pub blocks: Vec<Weak<IntermediateBlock>>,
    pub statements: Vec<IntermediateStatement>,
    pub next_signal_id: u32,

    pub input_signal_names: Vec<String>,
    pub output_signal_names: Vec<String>,
}

impl fmt::Display for IntermediateBlock {
    fn fmt(&self, f: &mut fmt::Formatter) -> Result<(), fmt::Error> {
        writeln!(f, "{}:", self.name)?;

        writeln!(f, "  Inputs: {}", self.input_signal_count)?;
        writeln!(f, "  Outputs: {}", self.output_signal_count)?;

        if self.blocks.len() > 0 {
            writeln!(f, "  Blocks:")?;
            for block_weak in &self.blocks {
                if let Some(block) = block_weak.upgrade() {
                    writeln!(f, "    {} ({} inputs, {} outputs)", block.name, block.input_signal_count, block.output_signal_count)?;
                } else {
                    writeln!(f, "    (dropped)")?;
                }
            }
        }

        for stmt in &self.statements {
            writeln!(f, "  {}", stmt)?;
        }

        Ok(())
    }
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
            input_signal_names: Vec::new(),
            output_signal_names: Vec::new(),
        }
    }

    pub fn allocate_signals(&mut self, count: u32) -> u32 {
        let base_signal_id = self.next_signal_id;
        self.next_signal_id += count;
        base_signal_id
    }

    pub fn allocate_input_signals(&mut self, count: u32, names: &Vec<String>) -> Result<u32, Error> {
        if self.output_signal_count > 0 || !self.blocks.is_empty() || !self.statements.is_empty() {
            return Err(ErrorKind::NoMoreInputSignals.into());
        }

        assert_eq!(names.len(), count as usize);

        let base_signal_id = self.next_signal_id;
        self.input_signal_count += count;
        self.next_signal_id += count;

        self.input_signal_names.extend(names.iter().cloned());

        Ok(base_signal_id)
    }

    pub fn allocate_output_signals(&mut self, count: u32, names: &Vec<String>) -> Result<u32, Error> {
        if !self.blocks.is_empty() || !self.statements.is_empty() {
            return Err(ErrorKind::NoMoreOutputSignals.into());
        }

        assert_eq!(names.len(), count as usize);

        let base_signal_id = self.next_signal_id;
        self.output_signal_count += count;
        self.next_signal_id += count;

        self.output_signal_names.extend(names.iter().cloned());

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

    pub fn optimize(&mut self) {
        // Stores the groups of signals which are directly connected
        let mut signal_groups: Vec<Vec<u32>> = Vec::new();

        for stmt in &self.statements {
            if let IntermediateOp::Connect = stmt.op {
                let signal1 = stmt.input_signal_ids[0];
                let signal2 = stmt.output_signal_ids[0];

                let mut group1_index = None;
                let mut group2_index = None;

                // Are the two signals already contained in groups?
                for (i, group) in signal_groups.iter().enumerate() {
                    if group.contains(&signal1) {
                        group1_index = Some(i);
                    }
                    if group.contains(&signal2) {
                        group2_index = Some(i);
                    }
                }

                if group1_index.is_some() && group2_index.is_some() {
                    // The two signals are already contained in groups
                    // This means that the connection connects two already existing groups,
                    // which we therefore have to merge

                    let group1_index = group1_index.unwrap();
                    let group2_index = group2_index.unwrap();

                    if group1_index != group2_index {
                        let group2 = signal_groups[group2_index].clone();
                        let group1 = &mut signal_groups[group1_index];

                        // Append elements from group 2 to group 1
                        group1.extend(group2.iter());

                        signal_groups.remove(group2_index);
                    }
                } else if group1_index.is_some() {
                    // Only signal 1 is contained in a group
                    // -> add signal 2 to this group

                    let group1_index = group1_index.unwrap();

                    signal_groups[group1_index].push(signal2);
                } else if group2_index.is_some() {
                    // Only signal 2 is contained in a group
                    // -> add signal 1 to this group

                    let group2_index = group2_index.unwrap();

                    signal_groups[group2_index].push(signal1);
                } else {
                    // No signal is contained in a group
                    // -> create a new group with both signals in it

                    signal_groups.push(vec![signal1, signal2]);
                }
            } else {
                // Create new groups for all signals used in this statement that aren't in any group

                let all_signals = stmt.input_signal_ids.iter().chain(stmt.output_signal_ids.iter());
                for signal in all_signals {
                    let is_in_group = signal_groups.iter()
                        .any(|group| group.contains(signal));

                    if !is_in_group {
                        signal_groups.push(vec![*signal]);
                    }
                }
            }
        }

        let mut group_signals = Vec::new();

        // We have to keep these signals because they are the public interface to other blocks
        let io_count = self.input_signal_count + self.output_signal_count;
        // We also can't change the connection signals to other blocks easily
        let block_signal_count = self.blocks.iter()
            .map(|block_weak| {
                let block = block_weak.upgrade().unwrap();
                block.input_signal_count + block.output_signal_count
            })
            .sum::<u32>();
        // These first N signals will not be eliminated
        let reserved_signal_count = io_count + block_signal_count;

        // Determine the new signals, one for each group
        let mut next_group_signal = reserved_signal_count;
        for group in &mut signal_groups {
            let mut group_signal: Option<u32> = None;

            // Check if any of the signals in this group is a reserved signal. If there is one, it
            // will be the new signal for the whole group. If there are multiple reserved signals,
            // only the first one is used. The other ones are removed from the group so that
            // reserved signals remain connected.
            group.retain(|&signal| {
                // Is the signal reserved?
                if signal < reserved_signal_count {
                    if group_signal.is_some() {
                        // We already came across a reserved signal -> remove this one from the group
                        false
                    } else {
                        group_signal = Some(signal);

                        true
                    }
                } else {
                    true
                }
            });

            // If there was no reserved signal in the group, assign a new signal ID to this group
            let group_signal = group_signal.unwrap_or_else(|| {
                let tmp = next_group_signal;
                next_group_signal += 1;
                tmp
            });

            group_signals.push(group_signal);
        }

        // Remove connection statements that connect two signals in the same group, they are
        // redundant now
        self.statements.retain(|stmt| {
            if let IntermediateOp::Connect = stmt.op {
                for group in &signal_groups {
                    if group.contains(&stmt.input_signal_ids[0]) && group.contains(&stmt.output_signal_ids[0]) {
                        return false;
                    }
                }
            }

            true
        });

        // Replace all occurrences of the old signals with the new group signals
        for stmt in &mut self.statements {
            let all_signals = stmt.input_signal_ids.iter_mut().chain(stmt.output_signal_ids.iter_mut());
            for signal in all_signals {
                for (group_index, group) in signal_groups.iter().enumerate() {
                    if group.contains(signal) {
                        *signal = group_signals[group_index];
                        break;
                    }
                }
            }
        }
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
