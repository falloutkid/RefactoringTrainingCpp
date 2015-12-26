#pragma once
#include"CommonData.h"

//! ���͂�o�́A�o�߂̌v�Z�ɗ��p����f�[�^�̃f�[�^�^�̎�ނ�\���܂��B
enum DATA_TYPE
{
	STRING,   //!< ������^�ł��B
	INTEGER,  //!< �����^�ł��B
	BOOLEAN   //!< �^�U�l�^�ł��B
};

//! ��̒l�����f�[�^�ł��B
class Data
{
public:
	Data();
	Data(const int init_data);
	Data(const bool init_data);
	Data(const char* init_data);

	enum DATA_TYPE type; //!< �f�[�^�̌^�ł��B

	char string_data[MAX_DATA_LENGTH]; //!< �f�[�^��������^�̏ꍇ�̒l�ł��B
	int integer;                  //!< �f�[�^�������^�̏ꍇ�̒l�ł��B
	bool boolean;                 //!< �f�[�^���^�U�l�^�̏ꍇ�̒l�ł��B
};
