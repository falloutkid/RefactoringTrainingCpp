#pragma once
#include "CommonData.h"
//! �g�[�N����\���܂��B
class Token
{
public:
	Token();
	Token(const TOKEN_KIND kind, const char* word);
	TOKEN_KIND kind_; //!< �g�[�N���̎�ނł��B
	char word_[MAX_WORD_LENGTH]; //!< �L�^����Ă���g�[�N���̕�����ł��B�L�^�̕K�v���Ȃ���΋󔒂ł��B
};