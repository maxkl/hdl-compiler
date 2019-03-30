
#include "SemanticAnalyzer.h"

#include "AST.h"
#include "SymbolTable.h"
#include "ExpressionResultType.h"

#include <shared/errors.h>

namespace HDLCompiler {

ExpressionResultType SemanticAnalyzer::analyzeBinaryExpression(AST::BinaryExpressionNode &expression, SymbolTable &symbolTable) {
	ExpressionResultType leftResultType, rightResultType;

	leftResultType = analyzeExpression(*expression.leftOperand, symbolTable);

	if (leftResultType.accessType != ExpressionResultType::AccessType::Read) {
		throw CompilerError("Write-only signal used as source operand");
	}

    rightResultType = analyzeExpression(*expression.rightOperand, symbolTable);

	if (rightResultType.accessType != ExpressionResultType::AccessType::Read) {
        throw CompilerError("Write-only signal used as source operand");
	}

	if (leftResultType.width != rightResultType.width) {
		throw CompilerError("Operand widths don't match");
	}

	return leftResultType;
}

ExpressionResultType SemanticAnalyzer::analyzeUnaryExpression(AST::UnaryExpressionNode &expression, SymbolTable &symbolTable) {
	ExpressionResultType resultType = analyzeExpression(*expression.operand, symbolTable);

	if (resultType.accessType != ExpressionResultType::AccessType::Read) {
		throw CompilerError("Write-only signal used as source operand");
	}

	return resultType;
}

ExpressionResultType SemanticAnalyzer::analyzeExpression(AST::ExpressionNode &expression, SymbolTable &symbolTable) {
    ExpressionResultType resultType;
	switch (expression.type) {
		case AST::ExpressionType::Binary:
            resultType = analyzeBinaryExpression(dynamic_cast<AST::BinaryExpressionNode &>(expression), symbolTable);
            break;
		case AST::ExpressionType::Unary:
            resultType = analyzeUnaryExpression(dynamic_cast<AST::UnaryExpressionNode &>(expression), symbolTable);
            break;
		case AST::ExpressionType::Variable:
			resultType = analyzeBehaviourIdentifier(*dynamic_cast<AST::VariableExpressionNode &>(expression).identifier, symbolTable);

			if (resultType.accessType != ExpressionResultType::AccessType::Read) {
				throw CompilerError("Write-only signal used as source operand");
			}

			break;
		case AST::ExpressionType::Constant:
		    resultType = ExpressionResultType(ExpressionResultType::AccessType::Read, dynamic_cast<AST::ConstantExpressionNode &>(expression).number->width);

			if (resultType.width == 0) {
                throw CompilerError("Number literal without width specifier used in expression");
            }

			break;
		default:
            throw std::domain_error("AST Expression node has invalid type");
	}

    expression.resultType = resultType;

    return resultType;
}

std::tuple<std::uint64_t, std::uint64_t> SemanticAnalyzer::analyzeSubscript(AST::SubscriptNode &subscript, SymbolTable &symbolTable) {
    std::uint64_t startIndex = subscript.start->value;

    std::uint64_t endIndex;
    if (subscript.end) {
        endIndex = subscript.end->value;
    } else {
        endIndex = startIndex;
    }

	if (endIndex > startIndex) {
		throw CompilerError("Invalid subscript range: end before start");
	}

    subscript.startIndex = startIndex;
    subscript.endIndex = endIndex;

	return std::make_tuple(startIndex, endIndex);
}

ExpressionResultType SemanticAnalyzer::analyzeBehaviourIdentifier(AST::BehaviourIdentifierNode &behaviourIdentifier, SymbolTable &symbolTable) {
    std::string signalName = behaviourIdentifier.identifier->value;

    Symbol *symbol = symbolTable.findRecursive(signalName);
    if (symbol == nullptr) {
    	throw CompilerError("Use of undeclared identifier \"" + signalName + "\"");
    }

    Symbol::TypeData *symbolType = symbol->type.get();

    ExpressionResultType::AccessType accessType;

    AST::IdentifierNode *propertyIdentifier = behaviourIdentifier.propertyIdentifier.get();
    if (propertyIdentifier == nullptr) {
        switch (symbolType->type) {
            case Symbol::Type::In:
                accessType = ExpressionResultType::AccessType::Read;
                break;
            case Symbol::Type::Out:
                accessType = ExpressionResultType::AccessType::Write;
                break;
            case Symbol::Type::Block:
                throw CompilerError("Block used as signal");
        }
    } else {
    	if (symbolType->type != Symbol::Type::Block) {
    		throw CompilerError("Property access on signal");
    	}

        std::string propertyName = propertyIdentifier->value;

        Symbol *propertySymbol = symbolType->block->symbolTable->find(propertyName);

        if (propertySymbol == nullptr) {
            throw CompilerError("\"" + signalName + "\" has no property \"" + propertyName + "\"");
        }

        Symbol::TypeData *propertySymbolType = propertySymbol->type.get();

        switch (propertySymbolType->type) {
            case Symbol::Type::In:
                accessType = ExpressionResultType::AccessType::Write;
                break;
            case Symbol::Type::Out:
                accessType = ExpressionResultType::AccessType::Read;
                break;
            default:
                throw CompilerError("Property \"" + propertyName + "\" of \"" + signalName + "\" is not accessible from other blocks");
        }

        symbolType = propertySymbolType;
    }

    std::uint64_t width;

    AST::SubscriptNode *subscript = behaviourIdentifier.subscript.get();
    if (subscript == nullptr) {
    	width = symbolType->width;
    } else {
        uint64_t startIndex, endIndex;

	    std::tie(startIndex, endIndex) = analyzeSubscript(*subscript, symbolTable);

        if (startIndex >= symbolType->width) {
            throw CompilerError("Subscript index " + std::to_string(startIndex) + " exceeds type width " + std::to_string(symbolType->width));
        }

        width = startIndex - endIndex + 1;
	}

	return { accessType, width };
}

void SemanticAnalyzer::analyzeBehaviourStatement(AST::BehaviourStatementNode &behaviourStatement, SymbolTable &symbolTable) {
    ExpressionResultType targetType = analyzeBehaviourIdentifier(*behaviourStatement.behaviourIdentifier, symbolTable);

    if (targetType.accessType != ExpressionResultType::AccessType::Write) {
    	throw CompilerError("Read-only signal used as target operand");
    }

    ExpressionResultType sourceType = analyzeExpression(*behaviourStatement.expression, symbolTable);

    if (sourceType.accessType != ExpressionResultType::AccessType::Read) {
    	throw CompilerError("Write-only signal used as source operand");
    }

    if (targetType.width != sourceType.width) {
		throw CompilerError("Operand types of assignment expression don't match");
	}

    behaviourStatement.resultType = sourceType;
}

void SemanticAnalyzer::analyzeDeclarationIdentifier(AST::IdentifierNode &identifier, SymbolTable &symbolTable, std::shared_ptr<Symbol::TypeData> symbolType) {
    std::unique_ptr<Symbol> symbol = std::make_unique<Symbol>(identifier.value, symbolType);

	bool success = symbolTable.add(std::move(symbol));
    if (!success) {
        throw CompilerError("Duplicate declaration of signal \"" + identifier.value + "\"");
    }
}

std::shared_ptr<Symbol::TypeData> SemanticAnalyzer::analyzeType(AST::TypeNode &type, SymbolTable &symbolTable) {
    std::shared_ptr<Symbol::TypeData> symbolType;

	AST::TypeSpecifierNode &typeSpecifier = *type.typeSpecifier;
    switch (typeSpecifier.type) {
    	case AST::TypeSpecifierType::In:
    		symbolType = std::make_shared<Symbol::TypeData>(Symbol::Type::In, 0);
    		break;
		case AST::TypeSpecifierType::Out:
            symbolType = std::make_shared<Symbol::TypeData>(Symbol::Type::Out, 0);
    		break;
    	case AST::TypeSpecifierType::Block: {
            const std::string &blockName = dynamic_cast<AST::TypeSpecifierBlockNode &>(typeSpecifier).identifier->value;

            TypeSymbol *blockSymbol = symbolTable.findTypeRecursive(blockName);

            if (!blockSymbol) {
                throw CompilerError("There is no block named \"" + blockName + "\"");
            }

            if (blockSymbol->type != TypeSymbol::Type::Block) {
                throw CompilerError("\"" + blockName + "\" is not a block");
            }

            symbolType = std::make_shared<Symbol::TypeData>(blockSymbol->block, 0);

            break;
        }
    	default:
            throw CompilerError("Invalid type specifier");
    }

    if (!type.width) {
    	symbolType->width = 1;
    } else {
	    std::uint64_t width = type.width->value;

	    if (width == 0) {
            throw CompilerError("Signal declared with invalid width of 0");
	    }

	    symbolType->width = width;
	}

    return symbolType;
}

void SemanticAnalyzer::analyzeDeclaration(AST::DeclarationNode &declaration, SymbolTable &symbolTable) {
    std::shared_ptr<Symbol::TypeData> symbolType = analyzeType(*declaration.type, symbolTable);

    for (const auto &identifier : declaration.identifiers) {
        analyzeDeclarationIdentifier(*identifier, symbolTable, symbolType);
    }
}

void SemanticAnalyzer::analyzeBlock(std::shared_ptr<AST::BlockNode> block, std::shared_ptr<SymbolTable> &symbolTable) {
    std::string &name = block->identifier->value;

    symbolTable->addType(std::make_unique<TypeSymbol>(name, block));

    block->symbolTable = std::make_shared<SymbolTable>(symbolTable);

    for (const auto &declaration : block->declarations) {
        analyzeDeclaration(*declaration, *block->symbolTable);
    }

    for (const auto &behaviourStatement : block->behaviourStatements) {
        analyzeBehaviourStatement(*behaviourStatement, *block->symbolTable);
    }
}

void SemanticAnalyzer::analyze(AST::RootNode &ast) {
    ast.symbolTable = std::make_shared<SymbolTable>();
    for (const auto &block : ast.blocks) {
        analyzeBlock(block, ast.symbolTable);
    }
}

}
