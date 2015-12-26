#include "stdafx.h"
#include "Token.h"

#include <iostream>
#pragma warning(disable:4996)

Token::Token() {}

Token::Token(const TOKEN_KIND kind, const char* word) {
	kind_ = kind;
	strcpy(word_, word);
}
