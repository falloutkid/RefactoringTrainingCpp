#pragma once
#include "CommonData.h"
//! WHERE句に指定する演算子の情報を表します。
class Operator
{
public:
	Operator();
	Operator(const TOKEN_KIND token_kind, const int order);
	TOKEN_KIND kind_; //!< 演算子の種類を、演算子を記述するトークンの種類で表します。
	int order_; //!< 演算子の優先順位です。
};
