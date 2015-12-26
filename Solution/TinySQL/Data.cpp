#include "stdafx.h"
#include "Data.h"
#include <string>
#include <algorithm>

#pragma warning(disable:4996)

Data::Data() :string_data("")
{
	type = DATA_TYPE::STRING;
}

Data::Data(const int init_data) : string_data(""), integer(init_data), type(DATA_TYPE::INTEGER)
{}
Data::Data(const bool init_data) : string_data(""), boolean(init_data), type(DATA_TYPE::BOOLEAN)
{}
Data::Data(const char* init_data) : type(DATA_TYPE::STRING)
{
	strncpy(string_data, init_data, std::max(MAX_DATA_LENGTH, MAX_WORD_LENGTH));
}
