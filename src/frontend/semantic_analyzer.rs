
use std::rc::Rc;
use std::cell::RefCell;
use std::borrow::Borrow;
use std::collections::hash_map::Entry;
use std::cmp;

use derive_more::Display;
use matches::matches;

use super::ast::*;
use super::symbol_table::SymbolTable;
use super::symbol::{SymbolType, SymbolTypeSpecifier, Symbol};
use super::expression_type::{ExpressionType, AccessType};
use crate::shared::error;
use crate::frontend::cache::Cache;

#[derive(Debug, Display)]
pub enum ErrorKind {
    #[display(fmt = "block `{}` defined twice", _0)]
    DuplicateBlock(String),

    #[display(fmt = "use of undeclared identifier `{}`", _0)]
    UndeclaredIdentifier(String),

    #[display(fmt = "clock input declared in combinational block")]
    ClockInCombinationalBlock,

    #[display(fmt = "more than one clock input declared")]
    TooManyClocks,

    #[display(fmt = "missing clock input for sequential block")]
    MissingClock,

    #[display(fmt = "clock input has width greater than one")]
    ClockTooWide,

    #[display(fmt = "signal(s) declared with invalid width of 0")]
    ZeroWidth,

    #[display(fmt = "target operand is not writable")]
    TargetNotWritable,

    #[display(fmt = "source operand is not readable")]
    SourceNotReadable,

    #[display(fmt = "types of operands to {} operator are incompatible", _0)]
    IncompatibleOperandTypes(String),

    #[display(fmt = "block used as signal")]
    BlockAsSignal,

    #[display(fmt = "property access on signal")]
    PropertyAccessOnSignal,

    #[display(fmt = "property `{}` of block `{}` is not publicly accessible", _1, _0)]
    PrivateProperty(String, String),

    #[display(fmt = "upper subscript index exceeds type width: {} > {}", _0, _1)]
    SubscriptExceedsWidth(u64, u64),

    #[display(fmt = "subscript indices swapped: upper <= lower ({} <= {})", _0, _1)]
    SubscriptIndicesSwapped(u64, u64),

    #[display(fmt = "number literal without width used in expression")]
    NoWidth,

    #[display(fmt = "failed to add symbol")]
    AddSymbol,
}

pub type Error = error::Error<ErrorKind>;

//impl From<SymbolTableError> for SemanticAnalyzerError {
//    fn from(err: SymbolTableError) -> Self {
//        ErrorKind::SymbolTable(err)
//    }
//}

pub struct SemanticAnalyzer {
    //
}

impl SemanticAnalyzer {
    pub fn analyze(root: &mut RootNode, cache: &Cache) -> Result<(), Error> {
        Self::analyze_root(root, cache)
    }

    fn analyze_root(root: &mut RootNode, cache: &Cache) -> Result<(), Error> {
        for (index, block) in root.blocks.iter().enumerate() {
            let block_name = {
                let block_ref = RefCell::borrow_mut(block.borrow());
                block_ref.name.value.clone()
            };

            Self::analyze_block(block, root, cache)?;

            match root.blocks_map.entry(block_name.clone()) {
                Entry::Occupied(o) => return Err(ErrorKind::DuplicateBlock(o.key().to_string()).into()),
                Entry::Vacant(v) => {
                    v.insert(index);
                }
            }
        }

        Ok(())
    }

    fn analyze_block(block: &Rc<RefCell<BlockNode>>, root: &RootNode, cache: &Cache) -> Result<(), Error> {
        let symbol_table = Rc::new(RefCell::new(SymbolTable::new()));

        // Nested scope needed so that `block_ref` is dropped before `block` is used later on to prevent RefCell borrow panics
        {
            let mut block_ref = RefCell::borrow_mut(block.borrow());

            {
                let mut symbol_table_ref = RefCell::borrow_mut(symbol_table.borrow());

                let mut total_clock_count = 0;

                for declaration in &mut block_ref.declarations {
                    let clock_count = Self::analyze_declaration(declaration, &mut symbol_table_ref, root, cache)?;

                    total_clock_count += clock_count;
                }

                if block_ref.is_sequential {
                    if total_clock_count == 0 {
                        return Err(ErrorKind::MissingClock.into());
                    } else if total_clock_count > 1 {
                        return Err(ErrorKind::TooManyClocks.into());
                    }
                } else {
                    if total_clock_count != 0 {
                        return Err(ErrorKind::ClockInCombinationalBlock.into());
                    }
                }

                for behaviour_statement in &mut block_ref.behaviour_statements {
                    Self::analyze_behaviour_statement(behaviour_statement, &mut symbol_table_ref)?;
                }
            }

            block_ref.symbol_table = Some(symbol_table);
        }

        Ok(())
    }

    fn analyze_declaration(declaration: &DeclarationNode, symbol_table: &mut SymbolTable, root: &RootNode, cache: &Cache) -> Result<usize, Error> {
        let symbol_type = Self::analyze_type(declaration.typ.as_ref(), root, cache)?;

        let is_clock = matches!(symbol_type.specifier, SymbolTypeSpecifier::Clock(_));
        let mut clock_count: usize = 0;

        for name in &declaration.names {
            Self::analyze_declaration_identifier(name, symbol_table, &symbol_type)?;

            if is_clock {
                clock_count += 1;
            }
        }

        Ok(clock_count)
    }

    fn analyze_type(typ: &TypeNode, root: &RootNode, cache: &Cache) -> Result<SymbolType, Error> {
        let type_specifier = match typ.specifier.as_ref() {
            TypeSpecifierNode::Clock(edge_type) => SymbolTypeSpecifier::Clock(*edge_type),
            TypeSpecifierNode::In => SymbolTypeSpecifier::In,
            TypeSpecifierNode::Out => SymbolTypeSpecifier::Out,
            TypeSpecifierNode::Wire => SymbolTypeSpecifier::Wire,
            TypeSpecifierNode::Block(name) => {
                let name = &name.value;

                let block = root.find_block(name, cache)
                    .ok_or_else(|| ErrorKind::UndeclaredIdentifier(name.to_string()))?;

                SymbolTypeSpecifier::Block(Rc::downgrade(block))
            },
        };

        let width = if let Some(width_node) = &typ.width {
            let width = width_node.value;

            if width == 0 {
                return Err(ErrorKind::ZeroWidth.into());
            }

            width
        } else {
            1
        };

        if matches!(type_specifier, SymbolTypeSpecifier::Clock(_)) && width > 1 {
            return Err(ErrorKind::ClockTooWide.into());
        }

        Ok(SymbolType {
            specifier: type_specifier,
            width: width,
        })
    }

    fn analyze_declaration_identifier(name: &IdentifierNode, symbol_table: &mut SymbolTable, symbol_type: &SymbolType) -> Result<(), Error> {
        let name = &name.value;

        let symbol = Symbol {
            name: name.to_string(),
            typ: symbol_type.clone(),
            signal_id: 0,
        };

        symbol_table.add(symbol)
            .map_err(|err| Error::with_source(ErrorKind::AddSymbol, err))?;

        Ok(())
    }

    fn analyze_behaviour_statement(behaviour_statement: &mut BehaviourStatementNode, symbol_table: &SymbolTable) -> Result<(), Error> {
        let target_type = Self::analyze_behaviour_identifier(behaviour_statement.target.as_mut(), symbol_table)?;

        if !target_type.access_type.writable {
            return Err(ErrorKind::TargetNotWritable.into());
        }

        let source_type = Self::analyze_expression(behaviour_statement.source.as_mut(), symbol_table)?;

        if !source_type.access_type.readable {
            return Err(ErrorKind::SourceNotReadable.into());
        }

        if target_type.width != source_type.width {
            return Err(ErrorKind::IncompatibleOperandTypes("assignment".to_string()).into());
        }

        behaviour_statement.expression_type = Some(source_type);

        Ok(())
    }

    fn analyze_behaviour_identifier(behaviour_identifier: &mut BehaviourIdentifierNode, symbol_table: &SymbolTable) -> Result<ExpressionType, Error> {
        let symbol_name = &behaviour_identifier.name.value;

        let symbol = symbol_table.find(symbol_name)
            .ok_or_else(|| ErrorKind::UndeclaredIdentifier(symbol_name.to_string()))?;

        let mut symbol_type = &symbol.typ;

        // Declared here to extend their scope to function scope
        let block_refcell;
        let block;
        let block_symbol_table;

        let access_type = if let Some(property) = &behaviour_identifier.property {
            let block_weak = match &symbol_type.specifier {
                SymbolTypeSpecifier::Block(block) => block,
                _ => return Err(ErrorKind::PropertyAccessOnSignal.into()),
            };

            block_refcell = block_weak.upgrade().unwrap();
            block = RefCell::borrow(block_refcell.borrow());

            let property_name = &property.value;

            let block_symbol_table_refcell = block.symbol_table.as_ref().unwrap();
            block_symbol_table = RefCell::borrow(block_symbol_table_refcell.borrow());

            let property_symbol = block_symbol_table.find(property_name)
                .ok_or_else(|| ErrorKind::UndeclaredIdentifier(format!("{}.{}", symbol_name, property_name)))?;

            let property_symbol_type = &property_symbol.typ;

            let access_type = match property_symbol_type.specifier {
                SymbolTypeSpecifier::Clock(_) => AccessType::rw(),
                SymbolTypeSpecifier::In => AccessType::rw(),
                SymbolTypeSpecifier::Out => AccessType::r(),
                _ => return Err(ErrorKind::PrivateProperty(symbol_name.to_string(), property_name.to_string()).into()),
            };

            symbol_type = property_symbol_type;

            access_type
        } else {
            match symbol_type.specifier {
                SymbolTypeSpecifier::Clock(_) => AccessType::r(),
                SymbolTypeSpecifier::In => AccessType::r(),
                SymbolTypeSpecifier::Out => AccessType::rw(),
                SymbolTypeSpecifier::Wire => AccessType::rw(),
                SymbolTypeSpecifier::Block(_) => return Err(ErrorKind::BlockAsSignal.into()),
            }
        };

        let width = if let Some(subscript) = &mut behaviour_identifier.subscript {
            let (upper_index, lower_index) = Self::analyze_subscript(subscript)?;

            if upper_index > symbol_type.width {
                return Err(ErrorKind::SubscriptExceedsWidth(upper_index, symbol_type.width).into());
            }

            upper_index - lower_index
        } else {
            symbol_type.width
        };

        Ok(ExpressionType {
            access_type,
            width,
        })
    }

    fn analyze_subscript(subscript: &mut SubscriptNode) -> Result<(u64, u64), Error> {
        let lower_index = subscript.lower.value;

        let upper_index = if let Some(upper) = &subscript.upper {
            upper.value
        } else {
            lower_index + 1
        };

        if lower_index >= upper_index {
            return Err(ErrorKind::SubscriptIndicesSwapped(upper_index, lower_index).into());
        }

        subscript.upper_index = Some(upper_index);
        subscript.lower_index = Some(lower_index);

        Ok((upper_index, lower_index))
    }

    fn analyze_expression(expression: &mut ExpressionNode, symbol_table: &SymbolTable) -> Result<ExpressionType, Error> {
        let expression_type = match &mut expression.data {
            ExpressionNodeData::Binary(op, left, right) =>
                Self::analyze_binary_expression(*op, left, right, symbol_table)?,
            ExpressionNodeData::Unary(op, operand) =>
                Self::analyze_unary_expression(*op, operand, symbol_table)?,
            ExpressionNodeData::Variable(behaviour_identifier) => {
                let expression_type = Self::analyze_behaviour_identifier(behaviour_identifier.as_mut(), symbol_table)?;

                if !expression_type.access_type.readable {
                    return Err(ErrorKind::SourceNotReadable.into());
                }

                expression_type
            },
            ExpressionNodeData::Const(number) => {
                let width = number.width
                    .ok_or_else(|| Error::new(ErrorKind::NoWidth))?;

                ExpressionType {
                    access_type: AccessType::r(),
                    width
                }
            },
        };

        expression.typ = Some(expression_type.clone());

        Ok(expression_type)
    }

    fn analyze_unary_expression(_op: UnaryOp, operand: &mut ExpressionNode, symbol_table: &SymbolTable) -> Result<ExpressionType, Error> {
        let expression_type = Self::analyze_expression(operand, symbol_table)?;

        if !expression_type.access_type.readable {
            return Err(ErrorKind::SourceNotReadable.into());
        }

        Ok(expression_type)
    }

    fn analyze_binary_expression(_op: BinaryOp, left: &mut ExpressionNode, right: &mut ExpressionNode, symbol_table: &SymbolTable) -> Result<ExpressionType, Error> {
        let expression_type_left = Self::analyze_expression(left, symbol_table)?;

        if !expression_type_left.access_type.readable {
            return Err(ErrorKind::SourceNotReadable.into());
        }

        let expression_type_right = Self::analyze_expression(right, symbol_table)?;

        if !expression_type_right.access_type.readable {
            return Err(ErrorKind::SourceNotReadable.into());
        }

        let left_width = expression_type_left.width;
        let right_width = expression_type_right.width;

        // Allow one of the operands to be a single bit, in which case it will be applied to every bit of the other operand
        if left_width != right_width && left_width != 1 && right_width != 1 {
            return Err(ErrorKind::IncompatibleOperandTypes("binary".to_string()).into());
        }

        Ok(ExpressionType {
            access_type: AccessType::r(),
            width: cmp::max(left_width, right_width),
        })
    }
}
