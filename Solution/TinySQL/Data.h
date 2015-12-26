#pragma once
#include"CommonData.h"

//! 入力や出力、経過の計算に利用するデータのデータ型の種類を表します。
enum DATA_TYPE
{
	STRING,   //!< 文字列型です。
	INTEGER,  //!< 整数型です。
	BOOLEAN   //!< 真偽値型です。
};

//! 一つの値を持つデータです。
class Data
{
public:
	Data();
	Data(const int init_data);
	Data(const bool init_data);
	Data(const char* init_data);

	enum DATA_TYPE type; //!< データの型です。

	char string_data[MAX_DATA_LENGTH]; //!< データが文字列型の場合の値です。
	int integer;                  //!< データが整数型の場合の値です。
	bool boolean;                 //!< データが真偽値型の場合の値です。
};
