
use std::rc::{Rc, Weak};
use std::cell::RefCell;
use std::borrow::Borrow;

use derive_more::Display;

use crate::shared::intermediate::{IntermediateBlock, IntermediateStatement, IntermediateOp, Intermediate};
use super::ast::*;
use super::symbol::SymbolTypeSpecifier;
use crate::frontend::symbol_table::SymbolTable;
use crate::frontend::expression_type::ExpressionType;
use crate::shared::{error, intermediate};

#[derive(Debug, Display)]
pub enum ErrorKind {
    #[display(fmt = "Intermediate")]
    Intermediate,
}

pub type Error = error::Error<ErrorKind>;

impl From<intermediate::Error> for Error {
    fn from(err: intermediate::Error) -> Self {
        Error::with_source(ErrorKind::Intermediate, err)
    }
}

pub struct IntermediateGenerator {
    //
}

#[derive(Copy, Clone)]
enum AccessDirection {
    Read,
    Write,
}

fn generate_signal_names(name: &str, count: usize) -> Vec<String> {
    if count == 1 {
        vec![name.to_string()]
    } else {
        (0..count)
            .map(|index| format!("{}[{}]", name, index))
            .collect()
    }
}

impl IntermediateGenerator {
    pub fn generate(root: &RootNode, optimization_level: u32) -> Result<Intermediate, Error> {
        Self::generate_blocks(root, optimization_level)
    }

    fn generate_blocks(root: &RootNode, optimization_level: u32) -> Result<Vec<Rc<IntermediateBlock>>, Error> {
        root.blocks.iter()
            .map(|block| Self::generate_block(&mut RefCell::borrow_mut(block), optimization_level))
            .collect()
    }

    fn generate_block(block: &mut BlockNode, optimization_level: u32) -> Result<Rc<IntermediateBlock>, Error> {
        let symbol_table_refcell = block.symbol_table.as_ref().unwrap();
        let mut symbol_table = RefCell::borrow_mut(symbol_table_refcell);

        let mut intermediate_block = IntermediateBlock::new(block.name.value.clone());

        for symbol in symbol_table.iter_mut() {
            if let SymbolTypeSpecifier::Clock(_) = symbol.typ.specifier {
                let signal_names = generate_signal_names(&symbol.name, symbol.typ.width as usize);
                let signal_id = intermediate_block.allocate_input_signals(symbol.typ.width as u32, &signal_names)?;
                symbol.base_signal_id = signal_id;
                symbol.output_base_signal_id = signal_id;
            }
        }

        for symbol in symbol_table.iter_mut() {
            if let SymbolTypeSpecifier::In = symbol.typ.specifier {
                let signal_names = generate_signal_names(&symbol.name, symbol.typ.width as usize);
                let signal_id = intermediate_block.allocate_input_signals(symbol.typ.width as u32, &signal_names)?;
                symbol.base_signal_id = signal_id;
                symbol.output_base_signal_id = signal_id;
            }
        }

        for symbol in symbol_table.iter_mut() {
            if let SymbolTypeSpecifier::Out = symbol.typ.specifier {
                let signal_names = generate_signal_names(&symbol.name, symbol.typ.width as usize);
                let signal_id = intermediate_block.allocate_output_signals(symbol.typ.width as u32, &signal_names)?;

                if block.is_sequential {
                    // symbol.base_signal_id will be set later when the output flip-flops are generated
                    symbol.output_base_signal_id = signal_id;
                } else {
                    symbol.base_signal_id = signal_id;
                    symbol.output_base_signal_id = signal_id;
                }
            }
        }

        for symbol in symbol_table.iter_mut() {
            if let SymbolTypeSpecifier::Block(block_weak) = &symbol.typ.specifier {
                let block_refcell = Weak::upgrade(block_weak).unwrap();
                let block = RefCell::borrow(block_refcell.borrow());
                let sub_intermediate_block = block.intermediate_block.as_ref().unwrap();
                symbol.base_signal_id = intermediate_block.add_block(Weak::clone(sub_intermediate_block))?;
                // symbol.output_base_signal_id is left untouched, as it doesn't make sense to specify it
            }
        }

        for symbol in symbol_table.iter_mut() {
            if let SymbolTypeSpecifier::Wire = symbol.typ.specifier {
                let signal_id = intermediate_block.allocate_signals(symbol.typ.width as u32);
                symbol.base_signal_id = signal_id;
                symbol.output_base_signal_id = signal_id;
            }
        }

        // Generate flip-flops for all output signals
        if block.is_sequential {
            // Find the clock input
            let (clock_signal_id_source, edge_type) = symbol_table.iter()
                .find_map(|symbol| match symbol.typ.specifier {
                    SymbolTypeSpecifier::Clock(edge_type) => Some((symbol.output_base_signal_id, edge_type)),
                    _ => None,
                })
                .unwrap();

            let clock_signal_id = match edge_type {
                EdgeType::Rising => clock_signal_id_source,
                EdgeType::Falling => {
                    let output_signal_id = intermediate_block.allocate_signals(1);

                    let mut stmt = IntermediateStatement::new(IntermediateOp::NOT, 1)?;
                    stmt.set_input(0, clock_signal_id_source);
                    stmt.set_output(0, output_signal_id);
                    intermediate_block.add_statement(stmt);

                    output_signal_id
                },
            };

            for symbol in symbol_table.iter_mut() {
                if let SymbolTypeSpecifier::Out = symbol.typ.specifier {
                    // This will be the input signal of the flip-flop
                    symbol.base_signal_id = intermediate_block.allocate_signals(symbol.typ.width as u32);

                    for j in 0..symbol.typ.width {
                        let mut stmt = IntermediateStatement::new(IntermediateOp::FlipFlop, 1)?;
                        // Connect the clock signal to the clock input
                        stmt.set_input(0, clock_signal_id);
                        // Connect the output of the combinational logic to the data input
                        stmt.set_input(1, symbol.base_signal_id + j as u32);
                        // Connect the flip-flops output to the output of the block
                        stmt.set_output(0, symbol.output_base_signal_id + j as u32);
                        intermediate_block.add_statement(stmt);
                    }
                }
            }
        }

        for behaviour_statement in &block.behaviour_statements {
            Self::generate_behaviour_statement(behaviour_statement, &symbol_table, &mut intermediate_block)?;
        }

        if optimization_level > 0 {
            intermediate_block.optimize();
        }

        let intermediate_block_rc = Rc::new(intermediate_block);

        block.intermediate_block = Some(Rc::downgrade(&intermediate_block_rc));

        Ok(intermediate_block_rc)
    }

    fn generate_behaviour_statement(behaviour_statement: &BehaviourStatementNode, symbol_table: &SymbolTable, intermediate_block: &mut IntermediateBlock) -> Result<(), Error> {
        let target_signal_id = Self::generate_behaviour_identifier(&behaviour_statement.target, symbol_table, AccessDirection::Write)?;

        let source_signal_id = Self::generate_expression(&behaviour_statement.source, symbol_table, intermediate_block)?;

        let expression_type = behaviour_statement.expression_type.as_ref().unwrap();
        let expression_width = expression_type.width;

        for i in 0..expression_width {
            let mut stmt = IntermediateStatement::new(IntermediateOp::Connect, 1)?;
            stmt.set_input(0, source_signal_id + i as u32);
            stmt.set_output(0, target_signal_id + i as u32);
            intermediate_block.add_statement(stmt);
        }

        Ok(())
    }

    fn generate_behaviour_identifier(behaviour_identifier: &BehaviourIdentifierNode, symbol_table: &SymbolTable, access_direction: AccessDirection) -> Result<u32, Error> {
        let symbol = symbol_table.find(&behaviour_identifier.name.value).unwrap();
        let symbol_type = &symbol.typ;

        let mut signal_id = if let Some(property_identifier) = &behaviour_identifier.property {
            let block_weak = match &symbol_type.specifier {
                SymbolTypeSpecifier::Block(block_weak) => block_weak,
                _ => panic!(),
            };

            let block_rc = Weak::upgrade(block_weak).unwrap();
            let block = RefCell::borrow(block_rc.borrow());

            let block_symbol_table = RefCell::borrow(block.symbol_table.as_ref().unwrap().borrow());

            let property_symbol = block_symbol_table.find(&property_identifier.value).unwrap();

            let offset = match access_direction {
                AccessDirection::Read => property_symbol.output_base_signal_id,
                AccessDirection::Write => property_symbol.base_signal_id,
            };

            symbol.base_signal_id + offset
        } else {
            match access_direction {
                AccessDirection::Read => symbol.output_base_signal_id,
                AccessDirection::Write => symbol.base_signal_id,
            }
        };

        if let Some(subscript) = &behaviour_identifier.subscript {
            signal_id += subscript.lower_index.unwrap() as u32;
        }

        Ok(signal_id)
    }

    fn generate_expression(expression: &ExpressionNode, symbol_table: &SymbolTable, intermediate_block: &mut IntermediateBlock) -> Result<u32, Error> {
        match &expression.data {
            ExpressionNodeData::Binary(op, left, right) => Self::generate_binary_expression(*op, left, right, expression.typ.as_ref().unwrap(), symbol_table, intermediate_block),
            ExpressionNodeData::Unary(op, operand) => Self::generate_unary_expression(*op, operand, expression.typ.as_ref().unwrap(), symbol_table, intermediate_block),
            ExpressionNodeData::Variable(behaviour_identifier) => Self::generate_behaviour_identifier(behaviour_identifier, symbol_table, AccessDirection::Read),
            ExpressionNodeData::Const(number) => Self::generate_constant(number.value, number.width.unwrap(), intermediate_block),
        }
    }

    fn generate_constant(value: u64, width: u64, intermediate_block: &mut IntermediateBlock) -> Result<u32, Error> {
        let output_signal_id = intermediate_block.allocate_signals(width as u32);

        for i in 0..width {
            let bit = (value & (1 << i)) != 0;
            let mut stmt = IntermediateStatement::new(if bit { IntermediateOp::Const1 } else { IntermediateOp::Const0 }, 1)?;
            stmt.set_output(0, output_signal_id + i as u32);
            intermediate_block.add_statement(stmt);
        }

        Ok(output_signal_id)
    }

    fn generate_unary_expression(op: UnaryOp, operand: &ExpressionNode, expression_type: &ExpressionType, symbol_table: &SymbolTable, intermediate_block: &mut IntermediateBlock) -> Result<u32, Error> {
        let expression_width = expression_type.width;

        let input_signal_id = Self::generate_expression(operand, symbol_table, intermediate_block)?;

        let output_signal_id = intermediate_block.allocate_signals(expression_width as u32);

        match op {
            UnaryOp::NOT => {
                for i in 0..expression_width {
                    let mut stmt = IntermediateStatement::new(IntermediateOp::NOT, 1)?;
                    stmt.set_input(0, input_signal_id + i as u32);
                    stmt.set_output(0, output_signal_id + i as u32);
                    intermediate_block.add_statement(stmt);
                }
            },
        }

        Ok(output_signal_id)
    }

    fn generate_binary_expression(op: BinaryOp, left: &ExpressionNode, right: &ExpressionNode, expression_type: &ExpressionType, symbol_table: &SymbolTable, intermediate_block: &mut IntermediateBlock) -> Result<u32, Error> {
        let expression_width = expression_type.width;

        let input_signal_id_left = Self::generate_expression(left, symbol_table, intermediate_block)?;

        let input_signal_id_right = Self::generate_expression(right, symbol_table, intermediate_block)?;

        let output_signal_id = intermediate_block.allocate_signals(expression_width as u32);

        match op {
            BinaryOp::AND |
            BinaryOp::OR |
            BinaryOp::XOR => {
                let intermediate_op = match op {
                    BinaryOp::AND => IntermediateOp::AND,
                    BinaryOp::OR => IntermediateOp::OR,
                    BinaryOp::XOR => IntermediateOp::XOR,
                    _ => panic!(),
                };

                for i in 0..expression_width {
                    let mut stmt = IntermediateStatement::new(intermediate_op, 2)?;

                    // One of the operands may only be a single bit wide, in which case it is applied to every bit

                    let input_a = if left.typ.as_ref().unwrap().width > 1 {
                        input_signal_id_left + i as u32
                    } else {
                        input_signal_id_left
                    };

                    let input_b = if right.typ.as_ref().unwrap().width > 1 {
                        input_signal_id_right + i as u32
                    } else {
                        input_signal_id_right
                    };

                    stmt.set_input(0, input_a);
                    stmt.set_input(1, input_b);

                    stmt.set_output(0, output_signal_id + i as u32);

                    intermediate_block.add_statement(stmt);
                }
            },
            BinaryOp::Add => {
                let mut prev_carry_signal_id = 0;

                for i in 0..expression_width {
                    let carry_signal_id = intermediate_block.allocate_signals(1);

                    let mut stmt = IntermediateStatement::new(
                        IntermediateOp::Add,
                        if i > 0 { 3 } else { 2 }
                    )?;

                    stmt.set_input(0, input_signal_id_left + i as u32);
                    stmt.set_input(1, input_signal_id_right + i as u32);
                    if i > 0 {
                        stmt.set_input(2, prev_carry_signal_id);
                    }

                    stmt.set_output(0, output_signal_id + i as u32);
                    stmt.set_output(1, carry_signal_id);

                    intermediate_block.add_statement(stmt);

                    prev_carry_signal_id = carry_signal_id;
                }
            },
        }

        Ok(output_signal_id)
    }
}
