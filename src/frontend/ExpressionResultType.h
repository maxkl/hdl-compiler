
#pragma once

#include <cstdint>

namespace HDLCompiler {

struct ExpressionResultType {
	enum class AccessType {
		Read,
		Write
	};

	AccessType accessType = AccessType::Read;
	std::uint64_t width = 0;

	ExpressionResultType() = default;
	ExpressionResultType(AccessType accessType, std::uint64_t width);

	friend bool operator==(const ExpressionResultType &a, const ExpressionResultType &b);
};

}
