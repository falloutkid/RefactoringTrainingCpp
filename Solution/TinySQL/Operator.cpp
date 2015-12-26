#include "stdafx.h"
#include "Operator.h"

Operator::Operator() {}

Operator::Operator(const TOKEN_KIND token_kind, const int order) {
	kind_ = token_kind;
	order_ = order;
}
