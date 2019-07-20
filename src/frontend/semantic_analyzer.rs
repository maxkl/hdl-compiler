
use std::rc::{Rc, Weak};
use std::cell::RefCell;
use std::borrow::Borrow;

use failure::Fail;

use super::ast::*;
use super::symbol_table::{SymbolTable, SymbolTableError};
use super::symbol::{SymbolType, SymbolTypeSpecifier, Symbol};
use super::expression_type::{ExpressionType, AccessType};

#[derive(Debug, Fail)]
pub enum SemanticAnalyzerError {
    #[fail(display = "use of undeclared identifier `{}`", _0)]
    UndeclaredIdentifier(String),

    #[fail(display = "symbol `{}` has invalid type: type `{}` required", _0, _1)]
    InvalidType(String, String),

    #[fail(display = "signal(s) declared with invalid width of 0")]
    ZeroWidth(),

    #[fail(display = "output signal used as target operand")]
    OutputAsTargetSignal(),

    #[fail(display = "input signal used as source operand")]
    InputAsSourceSignal(),

    #[fail(display = "types of operands to {} operator are incompatible", _0)]
    IncompatibleOperandTypes(String),

    #[fail(display = "block used as signal")]
    BlockAsSignal(),

    #[fail(display = "property access on signal")]
    PropertyAccessOnSignal(),

    #[fail(display = "property `{}` of block `{}` is not publicly accessible", _1, _0)]
    PrivateProperty(String, String),

    #[fail(display = "upper subscript index exceeds type width: {} > {}", _0, _1)]
    SubscriptExceedsWidth(u64, u64),

    #[fail(display = "subscript indices swapped: upper <= lower ({} <= {})", _0, _1)]
    SubscriptIndicesSwapped(u64, u64),

    #[fail(display = "number literal without width used in expression")]
    NoWidth(),

    #[fail(display = "{}", _0)]
    SymbolTable(#[cause] SymbolTableError),
}

impl From<SymbolTableError> for SemanticAnalyzerError {
    fn from(err: SymbolTableError) -> Self {
        SemanticAnalyzerError::SymbolTable(err)
    }
}

pub struct SemanticAnalyzer {
    //
}

impl SemanticAnalyzer {
    pub fn analyze(root: &mut RootNode) -> Result<(), SemanticAnalyzerError> {
        Self::analyze_root(root)
    }

    fn analyze_root(root: &mut RootNode) -> Result<(), SemanticAnalyzerError> {
        let global_symbol_table = Rc::new(RefCell::new(SymbolTable::new(None)));

        for block in &root.blocks {
            Self::analyze_block(block, &global_symbol_table)?;
        }

        root.symbol_table = Some(global_symbol_table);

        Ok(())
    }

    fn analyze_block(block: &Rc<RefCell<BlockNode>>, global_symbol_table: &Rc<RefCell<SymbolTable>>) -> Result<(), SemanticAnalyzerError> {
        let parent_weak = Rc::downgrade(global_symbol_table);

        let symbol_table = Rc::new(RefCell::new(SymbolTable::new(Some(parent_weak))));

        // Nested scope needed so that `block_ref` is dropped before `block` is used later on to prevent RefCell borrow panics
        {
            let mut block_ref = RefCell::borrow_mut(block.borrow());

            {
                let mut symbol_table_ref = RefCell::borrow_mut(symbol_table.borrow());

                for declaration in &mut block_ref.declarations {
                    Self::analyze_declaration(declaration, &mut symbol_table_ref)?;
                }

                for behaviour_statement in &mut block_ref.behaviour_statements {
                    Self::analyze_behaviour_statement(behaviour_statement, &mut symbol_table_ref)?;
                }
            }

            block_ref.symbol_table = Some(symbol_table);
        }

        let typ = Rc::new(SymbolType {
            specifier: SymbolTypeSpecifier::Block(Rc::downgrade(block)),
            width: 0
        });

        let mut global_symbol_table_ref = RefCell::borrow_mut(global_symbol_table.borrow());
        global_symbol_table_ref.add_type(typ)?;

        Ok(())
    }

    fn analyze_declaration(declaration: &DeclarationNode, symbol_table: &mut SymbolTable) -> Result<(), SemanticAnalyzerError> {
        let symbol_type = Self::analyze_type(declaration.typ.as_ref(), symbol_table)?;

        for name in &declaration.names {
            Self::analyze_declaration_identifier(name, symbol_table, &symbol_type)?;
        }

        Ok(())
    }

    fn analyze_type(typ: &TypeNode, symbol_table: &SymbolTable) -> Result<SymbolType, SemanticAnalyzerError> {
        let type_specifier = match typ.specifier.as_ref() {
            TypeSpecifierNode::In => SymbolTypeSpecifier::In,
            TypeSpecifierNode::Out => SymbolTypeSpecifier::Out,
            TypeSpecifierNode::Block(name) => {
                let name = &name.value;

                let block_symbol_type = symbol_table.find_type_recursive(name)
                    .ok_or_else(|| SemanticAnalyzerError::UndeclaredIdentifier(name.to_string()))?;

                let block = match &block_symbol_type.specifier {
                    SymbolTypeSpecifier::Block(block) => block,
                    _ => return Err(SemanticAnalyzerError::InvalidType(name.to_string(), "block".to_string())),
                };

                SymbolTypeSpecifier::Block(Weak::clone(block))
            },
        };

        let width = if let Some(width_node) = &typ.width {
            let width = width_node.value;

            if width == 0 {
                return Err(SemanticAnalyzerError::ZeroWidth());
            }

            width
        } else {
            1
        };

        Ok(SymbolType {
            specifier: type_specifier,
            width: width,
        })
    }

    fn analyze_declaration_identifier(name: &IdentifierNode, symbol_table: &mut SymbolTable, symbol_type: &SymbolType) -> Result<(), SemanticAnalyzerError> {
        let name = &name.value;

        let symbol = Rc::new(Symbol {
            name: name.to_string(),
            typ: symbol_type.clone(),
        });

        symbol_table.add(symbol)?;

        Ok(())
    }

    fn analyze_behaviour_statement(behaviour_statement: &mut BehaviourStatementNode, symbol_table: &SymbolTable) -> Result<(), SemanticAnalyzerError> {
        let target_type = Self::analyze_behaviour_identifier(behaviour_statement.target.as_mut(), symbol_table)?;

        if target_type.access_type != AccessType::Write {
            return Err(SemanticAnalyzerError::OutputAsTargetSignal());
        }

        let source_type = Self::analyze_expression(behaviour_statement.source.as_mut(), symbol_table)?;

        if source_type.access_type != AccessType::Read {
            return Err(SemanticAnalyzerError::InputAsSourceSignal());
        }

        if target_type.width != source_type.width {
            return Err(SemanticAnalyzerError::IncompatibleOperandTypes("assignment".to_string()));
        }

        behaviour_statement.expression_type = Some(source_type);

        Ok(())
    }

    fn analyze_behaviour_identifier(behaviour_identifier: &mut BehaviourIdentifierNode, symbol_table: &SymbolTable) -> Result<ExpressionType, SemanticAnalyzerError> {
        let symbol_name = &behaviour_identifier.name.value;

        let symbol = symbol_table.find_recursive(symbol_name)
            .ok_or_else(|| SemanticAnalyzerError::UndeclaredIdentifier(symbol_name.to_string()))?;

        let mut symbol_type = &symbol.typ;

        // Declared here to extend their scope to function scope
        let block_refcell;
        let block;
        let block_symbol_table;

        let access_type = if let Some(property) = &behaviour_identifier.property {
            let block_weak = match &symbol_type.specifier {
                SymbolTypeSpecifier::Block(block) => block,
                _ => return Err(SemanticAnalyzerError::PropertyAccessOnSignal()),
            };

            block_refcell = block_weak.upgrade().unwrap();
            block = RefCell::borrow(block_refcell.borrow());

            let property_name = &property.value;

            let block_symbol_table_refcell = block.symbol_table.as_ref().unwrap();
            block_symbol_table = RefCell::borrow(block_symbol_table_refcell.borrow());

            let property_symbol = block_symbol_table.find(property_name)
                .ok_or_else(|| SemanticAnalyzerError::UndeclaredIdentifier(format!("{}.{}", symbol_name, property_name)))?;

            let property_symbol_type = &property_symbol.typ;

            let access_type = match property_symbol_type.specifier {
                SymbolTypeSpecifier::In => AccessType::Write,
                SymbolTypeSpecifier::Out => AccessType::Read,
                _ => return Err(SemanticAnalyzerError::PrivateProperty(symbol_name.to_string(), property_name.to_string())),
            };

            symbol_type = property_symbol_type;

            access_type
        } else {
            match symbol_type.specifier {
                SymbolTypeSpecifier::In => AccessType::Read,
                SymbolTypeSpecifier::Out => AccessType::Write,
                SymbolTypeSpecifier::Block(_) => return Err(SemanticAnalyzerError::BlockAsSignal()),
            }
        };

        let width = if let Some(subscript) = &mut behaviour_identifier.subscript {
            let (upper_index, lower_index) = Self::analyze_subscript(subscript)?;

            if upper_index > symbol_type.width {
                return Err(SemanticAnalyzerError::SubscriptExceedsWidth(upper_index, symbol_type.width));
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

    fn analyze_subscript(subscript: &mut SubscriptNode) -> Result<(u64, u64), SemanticAnalyzerError> {
        let lower_index = subscript.lower.value;

        let upper_index = if let Some(upper) = &subscript.upper {
            upper.value
        } else {
            lower_index + 1
        };

        if lower_index >= upper_index {
            return Err(SemanticAnalyzerError::SubscriptIndicesSwapped(upper_index, lower_index));
        }

        subscript.upper_index = Some(upper_index);
        subscript.lower_index = Some(lower_index);

        Ok((upper_index, lower_index))
    }

    fn analyze_expression(expression: &mut ExpressionNode, symbol_table: &SymbolTable) -> Result<ExpressionType, SemanticAnalyzerError> {
        let expression_type = match &mut expression.data {
            ExpressionNodeData::Binary(op, left, right) =>
                Self::analyze_binary_expression(*op, left, right, symbol_table)?,
            ExpressionNodeData::Unary(op, operand) =>
                Self::analyze_unary_expression(*op, operand, symbol_table)?,
            ExpressionNodeData::Variable(behaviour_identifier) => {
                let expression_type = Self::analyze_behaviour_identifier(behaviour_identifier.as_mut(), symbol_table)?;

                if expression_type.access_type != AccessType::Read {
                    return Err(SemanticAnalyzerError::InputAsSourceSignal());
                }

                expression_type
            },
            ExpressionNodeData::Const(number) => {
                let width = number.width
                    .ok_or_else(|| SemanticAnalyzerError::NoWidth())?;

                ExpressionType {
                    access_type: AccessType::Read,
                    width
                }
            },
        };

        expression.typ = Some(expression_type.clone());

        Ok(expression_type)
    }

    fn analyze_unary_expression(op: UnaryOp, operand: &mut ExpressionNode, symbol_table: &SymbolTable) -> Result<ExpressionType, SemanticAnalyzerError> {
        let expression_type = Self::analyze_expression(operand, symbol_table)?;

        if expression_type.access_type != AccessType::Read {
            return Err(SemanticAnalyzerError::InputAsSourceSignal());
        }

        Ok(expression_type)
    }

    fn analyze_binary_expression(op: BinaryOp, left: &mut ExpressionNode, right: &mut ExpressionNode, symbol_table: &SymbolTable) -> Result<ExpressionType, SemanticAnalyzerError> {
        let expression_type_left = Self::analyze_expression(left, symbol_table)?;

        if expression_type_left.access_type != AccessType::Read {
            return Err(SemanticAnalyzerError::InputAsSourceSignal());
        }

        let expression_type_right = Self::analyze_expression(right, symbol_table)?;

        if expression_type_right.access_type != AccessType::Read {
            return Err(SemanticAnalyzerError::InputAsSourceSignal());
        }

        if expression_type_left != expression_type_right {
            return Err(SemanticAnalyzerError::IncompatibleOperandTypes("binary".to_string()));
        }

        Ok(expression_type_left)
    }
}
