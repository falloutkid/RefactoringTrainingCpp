#pragma once
//! �g�[�N���̎�ނ�\���܂��B
enum TOKEN_KIND
{
	NOT_TOKEN,              //!< �g�[�N����\���܂���B
	ASC,                    //!< ASC�L�[���[�h�ł��B
	AND,                    //!< AND�L�[���[�h�ł��B
	BY,                     //!< BY�L�[���[�h�ł��B
	DESC,                   //!< DESC�L�[���[�h�ł��B
	FROM,                   //!< FROM�L�[���[�h�ł��B
	OR,                     //!< OR�L�[���[�h�ł��B
	ORDER,                  //!< ORDER�L�[���[�h�ł��B
	SELECT,                 //!< SELECT�L�[���[�h�ł��B
	WHERE,                  //!< WHERE�L�[���[�h�ł��B
	ASTERISK,               //!< �� �L���ł��B
	COMMA,                  //!< �C �L���ł��B
	CLOSE_PAREN,            //!< �j �L���ł��B
	DOT,                    //!< �D �L���ł��B
	EQUAL,                  //!< �� �L���ł��B
	GREATER_THAN,           //!< �� �L���ł��B
	GREATER_THAN_OR_EQUAL,  //!< ���� �L���ł��B
	LESS_THAN,              //!< �� �L���ł��B
	LESS_THAN_OR_EQUAL,     //!< ���� �L���ł��B
	MINUS,                  //!< �| �L���ł��B
	NOT_EQUAL,              //!< ���� �L���ł��B
	OPEN_PAREN,             //!< �i �L���ł��B
	PLUS,                   //!< �{ �L���ł��B
	SLASH,                  //!< �^ �L���ł��B
	IDENTIFIER,             //!< ���ʎq�ł��B
	INT_LITERAL,            //!< �������e�����ł��B
	STRING_LITERAL          //!< �����񃊃e�����ł��B
};

//! WHERE��Ɏw�肷�鉉�Z�q�̏���\���܂��B
class Operator
{
public:
	Operator();
	Operator(const TOKEN_KIND token_kind, const int order);
	TOKEN_KIND kind_; //!< ���Z�q�̎�ނ��A���Z�q���L�q����g�[�N���̎�ނŕ\���܂��B
	int order_; //!< ���Z�q�̗D�揇�ʂł��B
};
