
#include "ExpressionResultType.h"

namespace HDLCompiler {

ExpressionResultType::ExpressionResultType(ExpressionResultType::AccessType accessType, std::uint64_t width)
	: accessType(accessType), width(width) {
	//
}

bool operator==(const ExpressionResultType &a, const ExpressionResultType &b) {
	return a.accessType == b.accessType && a.width == b.width;
}

}
