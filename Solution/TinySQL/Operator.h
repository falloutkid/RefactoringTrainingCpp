#pragma once
#include "CommonData.h"
//! WHERE��Ɏw�肷�鉉�Z�q�̏���\���܂��B
class Operator
{
public:
	Operator();
	Operator(const TOKEN_KIND token_kind, const int order);
	TOKEN_KIND kind_; //!< ���Z�q�̎�ނ��A���Z�q���L�q����g�[�N���̎�ނŕ\���܂��B
	int order_; //!< ���Z�q�̗D�揇�ʂł��B
};
