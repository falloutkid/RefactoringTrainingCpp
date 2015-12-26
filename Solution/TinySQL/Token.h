#pragma once
#include "CommonData.h"
//! トークンを表します。
class Token
{
public:
	Token();
	Token(const TOKEN_KIND kind, const char* word);
	TOKEN_KIND kind_; //!< トークンの種類です。
	char word_[MAX_WORD_LENGTH]; //!< 記録されているトークンの文字列です。記録の必要がなければ空白です。
};