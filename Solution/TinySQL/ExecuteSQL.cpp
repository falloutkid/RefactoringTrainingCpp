//! @file
#include "stdafx.h"
#include "stdio.h"
#include "stdbool.h"
#include "stdlib.h"
#include "ctype.h"

#include "Data.h"
#include "Operator.h"

#include <iostream>
//#include <string>
#include <algorithm>

#pragma warning(disable:4996)

namespace {
	//!< 入出力されるデータに含まれる列の最大数です。
	const int MAX_COLUMN_COUNT = 10;
	//!< 入出力されるデータに含まれる行の最大数です。
	const int MAX_ROW_COUNT = 256;
	//!< CSVとして入力されるテーブルの最大数です。
	const int MAX_TABLE_COUNT = 8;
	//!< WHERE句に指定される式木のノードの最大数です。
	const int MAX_EXTENSION_TREE_NODE_COUNT = 256;
	//!< SQLに含まれるトークンの最大値です。
	const int MAX_TOKEN_COUNT = 255;
	//!< 読み込むファイルの一行の最大長です。
	const int MAX_FILE_LINE_LENGTH = 4096;
	// 全てのアルファベットの大文字小文字とアンダーバーです。
	const char *ALPHABET_AND_UNDERBAR = "_abcdefghijklmnopqrstuvwxzABCDEFGHIJKLMNOPQRSTUVWXYZ";
	// 全ての数字とアルファベットの大文字小文字とアンダーバーです。
	const char *ALPHABET_NUMBER_AND_UNDERBAR = "_abcdefghijklmnopqrstuvwxzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
	// 全ての符号と数字です。
	const char *SIGNED_AND_NUMBER = "+-0123456789";
	// 全ての数字です
	const char *NUMBER = "0123456789";
	// 全ての空白文字です。
	const char* SPACE = " \t\r\n";
}


//! カレントディレクトリにあるCSVに対し、簡易的なSQLを実行し、結果をファイルに出力します。
//! @param [in] sql 実行するSQLです。
//! @param[in] outputFileName SQLの実行結果をCSVとして出力するファイル名です。拡張子を含みます。
//! @return 実行した結果の状態です。
int ExecuteSQL(const char*, const char*);

//! ExecuteSQLの戻り値の種類を表します。
enum RESULT_VALUE
{
	OK = 0,                     //!< 問題なく終了しました。
	ERR_FILE_OPEN = 1,          //!< ファイルを開くことに失敗しました。
	ERR_FILE_WRITE = 2,         //!< ファイルに書き込みを行うことに失敗しました。
	ERR_FILE_CLOSE = 3,         //!< ファイルを閉じることに失敗しました。
	ERR_TOKEN_CANT_READ = 4,    //!< トークン解析に失敗しました。
	ERR_SQL_SYNTAX = 5,         //!< SQLの構文解析が失敗しました。
	ERR_BAD_COLUMN_NAME = 6,    //!< テーブル指定を含む列名が適切ではありません。
	ERR_WHERE_OPERAND_TYPE = 7, //!< 演算の左右の型が適切ではありません。
	ERR_CSV_SYNTAX = 8,         //!< CSVの構文解析が失敗しました。
	ERR_MEMORY_ALLOCATE = 9,    //!< メモリの取得に失敗しました。
	ERR_MEMORY_OVER = 10        //!< 用意したメモリ領域の上限を超えました。
};

//! トークンを表します。
class Token
{
public:
	enum TOKEN_KIND kind; //!< トークンの種類です。
	char word[MAX_WORD_LENGTH]; //!< 記録されているトークンの文字列です。記録の必要がなければ空白です。
};

//! 指定された列の情報です。どのテーブルに所属するかの情報も含みます。 
class Column
{
public:
	char table_name[MAX_WORD_LENGTH]; //!< 列が所属するテーブル名です。指定されていない場合は空文字列となります。
	char column_name[MAX_WORD_LENGTH]; //!< 指定された列の列名です。
};

//! WHERE句の条件の式木を表します。
class ExtensionTreeNode
{
public:
	ExtensionTreeNode *parent; //!< 親となるノードです。根の式木の場合はnullptrとなります。
	ExtensionTreeNode *left;   //!< 左の子となるノードです。自身が末端の葉となる式木の場合はnullptrとなります。
	Operator middle_operator;                   //!< 中置される演算子です。自身が末端のとなる式木の場合の種類はNOT_TOKENとなります。
	ExtensionTreeNode *right;  //!< 右の子となるノードです。自身が末端の葉となる式木の場合はnullptrとなります。
	bool inParen;                        //!< 自身がかっこにくるまれているかどうかです。
	int parenOpenBeforeClose;            //!< 木の構築中に0以外となり、自身の左にあり、まだ閉じてないカッコの開始の数となります。
	int signCoefficient;                 //!< 自身が葉にあり、マイナス単項演算子がついている場合は-1、それ以外は1となります。
	Column column;                       //!< 列場指定されている場合に、その列を表します。列指定ではない場合はcolumnNameが空文字列となります。
	bool calculated;                     //!< 式の値を計算中に、計算済みかどうかです。
	Data value;                          //!< 指定された、もしくは計算された値です。
};

//! 行の情報を入力のテーブルインデックス、列インデックスの形で持ちます。
class ColumnIndex
{
public:
	int table;  //!< 列が入力の何テーブル目の列かです。
	int column; //!< 列が入力のテーブルの何列目かです。
} ;

// 以上ヘッダに相当する部分。

namespace {
	// 演算子の情報です。
	const Operator OPERATOR_LIST[] =
	{
		Operator( ASTERISK, 1 ),
		Operator( SLASH, 1),
		Operator( PLUS, 2),
		Operator( MINUS, 2),
		Operator( EQUAL, 3),
		Operator( GREATER_THAN, 3),
		Operator( GREATER_THAN_OR_EQUAL, 3),
		Operator( LESS_THAN, 3),
		Operator( LESS_THAN_OR_EQUAL, 3),
		Operator( NOT_EQUAL, 3),
		Operator( AND, 4),
		Operator( OR, 5),
	};

	// キーワードをトークンとして認識するためのキーワード一覧情報です。
	const Token KEYWORD_CONDITIONS[] =
	{
		{ AND, "AND" },
		{ ASC, "ASC" },
		{ BY, "BY" },
		{ DESC, "DESC" },
		{ FROM, "FROM" },
		{ ORDER, "ORDER" },
		{ OR, "OR" },
		{ SELECT, "SELECT" },
		{ WHERE, "WHERE" }
	};

	// 記号をトークンとして認識するための記号一覧情報です。
	const Token SIGN_CONDITIONS[] =
	{
		{ GREATER_THAN_OR_EQUAL, ">=" },
		{ LESS_THAN_OR_EQUAL, "<=" },
		{ NOT_EQUAL, "<>" },
		{ ASTERISK, "*" },
		{ COMMA, "," },
		{ CLOSE_PAREN, ")" },
		{ DOT, "." },
		{ EQUAL, "=" },
		{ GREATER_THAN, ">" },
		{ LESS_THAN, "<" },
		{ MINUS, "-" },
		{ OPEN_PAREN, "(" },
		{ PLUS, "+" },
		{ SLASH, "/" },
	};
}

//! カレントディレクトリにあるCSVに対し、簡易的なSQLを実行し、結果をファイルに出力します。
//! @param [in] sql 実行するSQLです。
//! @param[in] outputFileName SQLの実行結果をCSVとして出力するファイル名です。拡張子を含みます。
//! @return 実行した結果の状態です。
//! @retval OK=0                      問題なく終了しました。
//! @retval ERR_FILE_OPEN=1           ファイルを開くことに失敗しました。
//! @retval ERR_FILE_WRITE=2          ファイルに書き込みを行うことに失敗しました。
//! @retval ERR_FILE_CLOSE=3          ファイルを閉じることに失敗しました。
//! @retval ERR_TOKEN_CANT_READ=4     トークン解析に失敗しました。
//! @retval ERR_SQL_SYNTAX=5          SQLの構文解析が失敗しました。
//! @retval ERR_BAD_COLUMN_NAME=6     テーブル指定を含む列名が適切ではありません。
//! @retval ERR_WHERE_OPERAND_TYPE=7  演算の左右の型が適切ではありません。
//! @retval ERR_CSV_SYNTAX=8          CSVの構文解析が失敗しました。
//! @retval ERR_MEMORY_ALLOCATE=9     メモリの取得に失敗しました。
//! @retval ERR_MEMORY_OVER=10        用意したメモリ領域の上限を超えました。
//! @details 
//! 参照するテーブルは、テーブル名.csvの形で作成します。                                                     @n
//! 一行目はヘッダ行で、その行に列名を書きます。                                                             @n
//! 前後のスペース読み飛ばしやダブルクォーテーションでくくるなどの機能はありません。                         @n
//! 列の型の定義はできないので、列のすべてのデータの値が数値として解釈できる列のデータを整数として扱います。 @n
//! 実行するSQLで使える機能を以下に例としてあげます。                                                        @n
//! 例1:                                                                                                     @n
//! SELECT *                                                                                                 @n
//! FROM USERS                                                                                               @n
//!                                                                                                          @n
//! 例2: 大文字小文字は区別しません。                                                                        @n
//! select *                                                                                                 @n
//! from users                                                                                               @n
//!                                                                                                          @n
//! 例3: 列の指定ができます。                                                                                @n
//! SELECT Id, Name                                                                                          @n
//! FROM USERS                                                                                               @n
//!                                                                                                          @n
//! 例4: テーブル名を指定して列の指定ができます。                                                            @n
//! SELECT USERS.Id                                                                                          @n
//! FROM USERS                                                                                               @n
//!                                                                                                          @n
//! 例5: ORDER句が使えます。                                                                                 @n
//! SELECT *                                                                                                 @n
//! ORDER BY NAME                                                                                            @n
//! FROM USERS                                                                                               @n
//!                                                                                                          @n
//! 例6: ORDER句に複数列や昇順、降順の指定ができます。                                                       @n
//! SELECT *                                                                                                 @n
//! ORDER BY AGE DESC, Name ASC                                                                              @n
//!                                                                                                          @n
//! 例7: WHERE句が使えます。                                                                                 @n
//! SELECT *                                                                                                 @n
//! WHERE AGE >= 20                                                                                          @n
//! FROM USERS                                                                                               @n
//!                                                                                                          @n
//! 例8: WHERE句では文字列の比較も使えます。                                                                 @n
//! SELECT *                                                                                                 @n
//! WHERE NAME >= 'N'                                                                                        @n
//! FROM USERS                                                                                               @n
//!                                                                                                          @n
//! 例9: WHERE句には四則演算、カッコ、AND、ORなどを含む複雑な式が利用できます。                              @n
//! SELECT *                                                                                                 @n
//! WHERE AGE >= 20 AND (AGE <= 40 || WEIGHT < 100)                                                          @n
//! FROM USERS                                                                                               @n
//!                                                                                                          @n
//! 例10: FROM句に複数のテーブルが指定できます。その場合はクロスで結合します。                               @n
//! SELECT *                                                                                                 @n
//! FROM USERS, CHILDREN                                                                                     @n
//!                                                                                                          @n
//! 例11: WHEREで条件をつけることにより、テーブルの結合ができます。                                          @n
//! SELECT USERS.NAME, CHILDREN.NAME                                                                         @n
//! WHERE USERS.ID = CHILDREN.PARENTID                                                                       @n
//! FROM USERS, CHILDREN                                                                                     @n
int ExecuteSQL(const char* sql, const char* output_file_name)
{
	enum RESULT_VALUE error = OK;                           // 発生したエラーの種類です。
	FILE *input_table_files[MAX_TABLE_COUNT] = { nullptr };      // 読み込む入力ファイルの全てのファイルポインタです。
	FILE *output_file = nullptr;                                // 書き込むファイルのファイルポインタです。
	int result = 0;                                         // 関数の戻り値を一時的に保存します。
	bool found = false;                                     // 検索時に見つかったかどうかの結果を一時的に保存します。
	const char *search = nullptr;                              // 文字列検索に利用するポインタです。
	Data ***current_row = nullptr;                              // データ検索時に現在見ている行を表します。
	Data **input_data[MAX_TABLE_COUNT][MAX_ROW_COUNT];       // 入力データです。
	Data **output_data[MAX_ROW_COUNT] = { nullptr };            // 出力データです。
	Data **all_column_output_data[MAX_ROW_COUNT] = { nullptr };   // 出力するデータに対応するインデックスを持ち、すべての入力データを保管します。

	// inputDataを初期化します。
	for (size_t table_index = 0; table_index < sizeof(input_data) / sizeof(input_data[0]); table_index++)
	{
		for (size_t row_index = 0; row_index < sizeof(input_data[0]) / sizeof(input_data[0][0]); row_index++){
			input_data[table_index][row_index] = nullptr;
		}
	}

	// SQLからトークンを読み込みます。

	// keywordConditionsとsignConditionsは先頭から順に検索されるので、前方一致となる二つの項目は順番に気をつけて登録しなくてはいけません。

	Token tokens[MAX_TOKEN_COUNT]; // SQLを分割したトークンです。
	int tokensNum = 0; // tokensの有効な数です。

	const char* charactorBackPoint = nullptr; // SQLをトークンに分割して読み込む時に戻るポイントを記録しておきます。

	const char* charactorCursol = sql; // SQLをトークンに分割して読み込む時に現在読んでいる文字の場所を表します。

	char tableNames[MAX_TABLE_COUNT][MAX_WORD_LENGTH]; // FROM句で指定しているテーブル名です。
	// tableNamesを初期化します。
	for (size_t i = 0; i < sizeof(tableNames) / sizeof(tableNames[0]); i++)
	{
		strncpy(tableNames[i], "", MAX_WORD_LENGTH);
	}
	int tableNamesNum = 0; // 現在読み込まれているテーブル名の数です。

	// SQLをトークンに分割て読み込みます。
	while (*charactorCursol){

		// 空白を読み飛ばします。
		for (search = SPACE; *search && *charactorCursol != *search; ++search){}
		if (*search){
			charactorCursol++;
			continue;
		}

		// 数値リテラルを読み込みます。

		// 先頭文字が数字であるかどうかを確認します。
		charactorBackPoint = charactorCursol;
		for (search = NUMBER; *search && *charactorCursol != *search; ++search){}
		if (*search){
			Token literal = {INT_LITERAL, "" }; // 読み込んだ数値リテラルの情報です。
			int wordLength = 0; // 数値リテラルに現在読み込んでいる文字の数です。

			// 数字が続く間、文字を読み込み続けます。
			do {
				for (search = NUMBER; *search && *charactorCursol != *search; ++search){}
				if (*search){
					if (MAX_WORD_LENGTH - 1 <= wordLength){
						error = ERR_MEMORY_OVER;
						goto ERROR;
					}
					literal.word[wordLength++] = *search;
					++charactorCursol;
				}
			} while (*search);

			// 数字の後にすぐに識別子が続くのは紛らわしいので数値リテラルとは扱いません。
			for (search = ALPHABET_AND_UNDERBAR; *search && *charactorCursol != *search; ++search){}
			if (!*search){
				literal.word[wordLength] = '\0';
				if (MAX_TOKEN_COUNT <= tokensNum){
					error = ERR_MEMORY_OVER;
					goto ERROR;
				}
				tokens[tokensNum++] = literal;
				continue;
			}
			else{
				charactorCursol = charactorBackPoint;
			}
		}

		// 文字列リテラルを読み込みます。

		// 文字列リテラルを開始するシングルクォートを判別し、読み込みます。
		// メトリクス測定ツールのccccはシングルクォートの文字リテラル中のエスケープを認識しないため、文字リテラルを使わないことで回避しています。
		if (*charactorCursol == "\'"[0]){
			++charactorCursol;
			Token literal = { STRING_LITERAL, "\'" }; // 読み込んだ文字列リテラルの情報です。
			int wordLength = 1; // 文字列リテラルに現在読み込んでいる文字の数です。初期値の段階で最初のシングルクォートは読み込んでいます。

			// 次のシングルクォートがくるまで文字を読み込み続けます。
			while (*charactorCursol && *charactorCursol != "\'"[0]){
				if (MAX_WORD_LENGTH - 1 <= wordLength){
					error = ERR_MEMORY_OVER;
					goto ERROR;
				}
				literal.word[wordLength++] = *charactorCursol++;
			}
			if (*charactorCursol == "\'"[0]){
				if (MAX_WORD_LENGTH - 1 <= wordLength){
					error = ERR_MEMORY_OVER;
					goto ERROR;
				}
				// 最後のシングルクォートを読み込みます。
				literal.word[wordLength++] = *charactorCursol++;

				// 文字列の終端文字をつけます。
				literal.word[wordLength] = '\0';
				if (MAX_TOKEN_COUNT <= tokensNum){
					error = ERR_MEMORY_OVER;
					goto ERROR;
				}
				tokens[tokensNum++] = literal;
				continue;
			}
			else{
				error = ERR_TOKEN_CANT_READ;
				goto ERROR;
			}
		}

		// キーワードを読み込みます。
		found = false;
		for (size_t i = 0; i < sizeof(KEYWORD_CONDITIONS) / sizeof(KEYWORD_CONDITIONS[0]); ++i){
			charactorBackPoint = charactorCursol;
			Token condition = KEYWORD_CONDITIONS[i]; // 確認するキーワードの条件です。
			char *wordCursol = condition.word; // 確認するキーワードの文字列のうち、現在確認している一文字を指します。

			// キーワードが指定した文字列となっているか確認します。
			while (*wordCursol && toupper(*charactorCursol++) == *wordCursol){
				++wordCursol;
			}

			// キーワードに識別子が区切りなしに続いていないかを確認するため、キーワードの終わった一文字あとを調べます。
			for (search = ALPHABET_NUMBER_AND_UNDERBAR; *search && *charactorCursol != *search; ++search){};

			if (!*wordCursol && !*search){

				// 見つかったキーワードを生成します。
				if (MAX_TOKEN_COUNT <= tokensNum){
					error = ERR_MEMORY_OVER;
					goto ERROR;
				}
				tokens[tokensNum++] = { condition.kind, "" };
				found = true;
			}
			else{
				charactorCursol = charactorBackPoint;
			}
		}
		if (found){
			continue;
		}

		// 記号を読み込みます。
		found = false;
		for (size_t i = 0; i < sizeof(SIGN_CONDITIONS) / sizeof(Token); ++i){
			charactorBackPoint = charactorCursol;
			Token condition = SIGN_CONDITIONS[i]; // 確認する記号の条件です。
			char *wordCursol = condition.word; // 確認する記号の文字列のうち、現在確認している一文字を指します。

			// 記号が指定した文字列となっているか確認します。
			while (*wordCursol && toupper(*charactorCursol++) == *wordCursol){
				++wordCursol;
			}
			if (!*wordCursol){

				// 見つかった記号を生成します。
				if (MAX_TOKEN_COUNT <= tokensNum){
					error = ERR_MEMORY_OVER;
					goto ERROR;
				}
				tokens[tokensNum++] = { condition.kind, "" };
				found = true;
			}
			else{
				charactorCursol = charactorBackPoint;
			}
		}
		if (found){
			continue;
		}

		// 識別子を読み込みます。

		// 識別子の最初の文字を確認します。
		for (search = ALPHABET_AND_UNDERBAR; *search && *charactorCursol != *search; ++search){};
		if (*search){
			Token identifier = { IDENTIFIER, "" }; // 読み込んだ識別子の情報です。
			int wordLength = 0; // 識別子に現在読み込んでいる文字の数です。
			do {
				// 二文字目以降は数字も許可して文字の種類を確認します。
				for (search = ALPHABET_NUMBER_AND_UNDERBAR; *search && *charactorCursol != *search; ++search){};
				if (*search){
					if (MAX_WORD_LENGTH - 1 <= wordLength){
						error = ERR_MEMORY_OVER;
						goto ERROR;
					}
					identifier.word[wordLength++] = *search;
					charactorCursol++;
				}
			} while (*search);

			// 識別子の文字列の終端文字を設定します。
			identifier.word[wordLength] = '\0';

			// 読み込んだ識別子を登録します。
			if (MAX_TOKEN_COUNT <= tokensNum){
				error = ERR_MEMORY_OVER;
				goto ERROR;
			}
			tokens[tokensNum++] = identifier;
			continue;
		}
		else{
			charactorCursol = charactorBackPoint;
		}

		error = ERR_TOKEN_CANT_READ;
		goto ERROR;
	}

	// トークン列を解析し、構文を読み取ります。

	Token *tokenCursol = tokens; // 現在見ているトークンを指します。

	Column selectColumns[MAX_TABLE_COUNT * MAX_COLUMN_COUNT]; // SELECT句に指定された列名です。
	// selectColumnsを初期化します。
	for (size_t i = 0; i < sizeof(selectColumns) / sizeof(selectColumns[0]); i++)
	{
		selectColumns[i] = {"", "" };
	}
	int selectColumnsNum = 0; // SELECT句から現在読み込まれた列名の数です。

	Column orderByColumns[MAX_COLUMN_COUNT]; // ORDER句に指定された列名です。
	// orderByColumnsを初期化します。
	for (size_t i = 0; i < sizeof(orderByColumns) / sizeof(orderByColumns[0]); i++)
	{
		orderByColumns[i] = { "", "" };
	}
	int orderByColumnsNum = 0; // ORDER句から現在読み込まれた列名の数です。

	enum TOKEN_KIND orders[MAX_COLUMN_COUNT] = { NOT_TOKEN }; // 同じインデックスのorderByColumnsに対応している、昇順、降順の指定です。

	ExtensionTreeNode whereExtensionNodes[MAX_EXTENSION_TREE_NODE_COUNT]; // WHEREに指定された木のノードを、木構造とは無関係に格納します。
	// whereExtensionNodesを初期化します。
	for (size_t i = 0; i < sizeof(whereExtensionNodes) / sizeof(whereExtensionNodes[0]); i++)
	{
		whereExtensionNodes[i] = {
			nullptr,
			nullptr,
			{ NOT_TOKEN, 0 },
			nullptr,
			false,
			0,
			1,
			{"", "" },
			false,
			Data(),
		};
	}
	int whereExtensionNodesNum = 0; // 現在読み込まれているのwhereExtensionNodesの数です。

	ExtensionTreeNode *whereTopNode = nullptr; // 式木の根となるノードです。

	// SQLの構文を解析し、必要な情報を取得します。

	// SELECT句を読み込みます。
	if (tokenCursol->kind == SELECT){
		++tokenCursol;
	}
	else{
		error = ERR_SQL_SYNTAX;
		goto ERROR;
	}

	if (tokenCursol->kind == ASTERISK){
		++tokenCursol;
	}
	else
	{
		bool first = true; // SELECT句に最初に指定された列名の読み込みかどうかです。
		while (tokenCursol->kind == COMMA || first){
			if (tokenCursol->kind == COMMA){
				++tokenCursol;
			}
			if (tokenCursol->kind == IDENTIFIER){
				if (MAX_COLUMN_COUNT <= selectColumnsNum){
					error = ERR_MEMORY_OVER;
					goto ERROR;
				}
				// テーブル名が指定されていない場合と仮定して読み込みます。
				strncpy(selectColumns[selectColumnsNum].table_name, "", MAX_WORD_LENGTH);
				strncpy(selectColumns[selectColumnsNum].column_name, tokenCursol->word, MAX_WORD_LENGTH);
				++tokenCursol;
				if (tokenCursol->kind == DOT){
					++tokenCursol;
					if (tokenCursol->kind == IDENTIFIER){

						// テーブル名が指定されていることがわかったので読み替えます。
						strncpy(selectColumns[selectColumnsNum].table_name, selectColumns[selectColumnsNum].column_name, MAX_WORD_LENGTH);
						strncpy(selectColumns[selectColumnsNum].column_name, tokenCursol->word, MAX_WORD_LENGTH);
						++tokenCursol;
					}
					else{
						error = ERR_SQL_SYNTAX;
						goto ERROR;
					}
				}
				++selectColumnsNum;
			}
			else{
				error = ERR_SQL_SYNTAX;
				goto ERROR;
			}
			first = false;
		}
	}

	// ORDER句とWHERE句を読み込みます。最大各一回ずつ書くことができます。
	bool readOrder = false; // すでにORDER句が読み込み済みかどうかです。
	bool readWhere = false; // すでにWHERE句が読み込み済みかどうかです。
	while (tokenCursol->kind == ORDER || tokenCursol->kind == WHERE){

		// 二度目のORDER句はエラーです。
		if (readOrder && tokenCursol->kind == ORDER){
			error = ERR_SQL_SYNTAX;
			goto ERROR;
		}

		// 二度目のWHERE句はエラーです。
		if (readWhere && tokenCursol->kind == WHERE){
			error = ERR_SQL_SYNTAX;
			goto ERROR;
		}
		// ORDER句を読み込みます。
		if (tokenCursol->kind == ORDER){
			readOrder = true;
			++tokenCursol;
			if (tokenCursol->kind == BY){
				++tokenCursol;
				bool first = true; // ORDER句の最初の列名の読み込みかどうかです。
				while (tokenCursol->kind == COMMA || first){
					if (tokenCursol->kind == COMMA){
						++tokenCursol;
					}
					if (tokenCursol->kind == IDENTIFIER){
						if (MAX_COLUMN_COUNT <= orderByColumnsNum){
							error = ERR_MEMORY_OVER;
							goto ERROR;
						}
						// テーブル名が指定されていない場合と仮定して読み込みます。
						strncpy(orderByColumns[orderByColumnsNum].table_name, "", MAX_WORD_LENGTH);
						strncpy(orderByColumns[orderByColumnsNum].column_name, tokenCursol->word, MAX_WORD_LENGTH);
						++tokenCursol;
						if (tokenCursol->kind == DOT){
							++tokenCursol;
							if (tokenCursol->kind == IDENTIFIER){

								// テーブル名が指定されていることがわかったので読み替えます。
								strncpy(orderByColumns[orderByColumnsNum].table_name, orderByColumns[orderByColumnsNum].column_name, MAX_WORD_LENGTH);
								strncpy(orderByColumns[orderByColumnsNum].column_name, tokenCursol->word, MAX_WORD_LENGTH);
								++tokenCursol;
							}
							else{
								error = ERR_SQL_SYNTAX;
								goto ERROR;
							}
						}

						// 並び替えの昇順、降順を指定します。
						if (tokenCursol->kind == ASC){
							orders[orderByColumnsNum] = ASC;
							++tokenCursol;
						}
						else if (tokenCursol->kind == DESC){
							orders[orderByColumnsNum] = DESC;
							++tokenCursol;
						}
						else{
							// 指定がない場合は昇順となります。
							orders[orderByColumnsNum] = ASC;
						}
						++orderByColumnsNum;
					}
					else{
						error = ERR_SQL_SYNTAX;
						goto ERROR;
					}
					first = false;
				}
			}
			else{
				error = ERR_SQL_SYNTAX;
				goto ERROR;
			}
		}

		// WHERE句を読み込みます。
		if (tokenCursol->kind == WHERE){
			readWhere = true;
			++tokenCursol;
			ExtensionTreeNode *currentNode = nullptr; // 現在読み込んでいるノードです。
			while (true){
				// オペランドを読み込みます。

				// オペランドのノードを新しく生成します。
				if (MAX_EXTENSION_TREE_NODE_COUNT <= whereExtensionNodesNum){
					error = ERR_MEMORY_OVER;
					goto ERROR;
				}
				if (currentNode){
					// 現在のノードを右の子にずらし、元の位置に新しいノードを挿入します。
					currentNode->right = &whereExtensionNodes[whereExtensionNodesNum++];
					currentNode->right->parent = currentNode;
					currentNode = currentNode->right;
				}
				else{
					// 最初はカレントノードに新しいノードを入れます。
					currentNode = &whereExtensionNodes[whereExtensionNodesNum++];
				}

				// カッコ開くを読み込みます。
				while (tokenCursol->kind == OPEN_PAREN){
					++currentNode->parenOpenBeforeClose;
					++tokenCursol;
				}

				// オペランドに前置される+か-を読み込みます。
				if (tokenCursol->kind == PLUS || tokenCursol->kind == MINUS){

					// +-を前置するのは列名と数値リテラルのみです。
					if (tokenCursol[1].kind != IDENTIFIER && tokenCursol[1].kind != INT_LITERAL){
						error = ERR_WHERE_OPERAND_TYPE;
						goto ERROR;
					}
					if (tokenCursol->kind == MINUS){
						currentNode->signCoefficient = -1;
					}
					++tokenCursol;
				}

				// 列名、整数リテラル、文字列リテラルのいずれかをオペランドとして読み込みます。
				if (tokenCursol->kind == IDENTIFIER){

					// テーブル名が指定されていない場合と仮定して読み込みます。
					strncpy(currentNode->column.table_name, "", MAX_WORD_LENGTH);
					strncpy(currentNode->column.column_name, tokenCursol->word, MAX_WORD_LENGTH);
					++tokenCursol;
					if (tokenCursol->kind == DOT){
						++tokenCursol;
						if (tokenCursol->kind == IDENTIFIER){

							// テーブル名が指定されていることがわかったので読み替えます。
							strncpy(currentNode->column.table_name, currentNode->column.column_name, MAX_WORD_LENGTH);
							strncpy(currentNode->column.column_name, tokenCursol->word, MAX_WORD_LENGTH);
							++tokenCursol;
						}
						else{
							error = ERR_SQL_SYNTAX;
							goto ERROR;
						}
					}
				}
				else if (tokenCursol->kind == INT_LITERAL){
					currentNode->value = Data(atoi(tokenCursol->word));
					++tokenCursol;
				}
				else if (tokenCursol->kind == STRING_LITERAL){
					currentNode->value = Data("");

					// 前後のシングルクォートを取り去った文字列をデータとして読み込みます。
					strncpy(currentNode->value.string_data, tokenCursol->word + 1, std::min(MAX_WORD_LENGTH, MAX_DATA_LENGTH));
					currentNode->value.string_data[MAX_DATA_LENGTH - 1] = '\0';
					currentNode->value.string_data[strlen(currentNode->value.string_data) - 1] = '\0';
					++tokenCursol;
				}
				else{
					error = ERR_SQL_SYNTAX;
					goto ERROR;
				}

				// オペランドの右のカッコ閉じるを読み込みます。
				while (tokenCursol->kind == CLOSE_PAREN){
					ExtensionTreeNode *searchedAncestor = currentNode->parent; // カッコ閉じると対応するカッコ開くを両方含む祖先ノードを探すためのカーソルです。
					while (searchedAncestor){

						// searchedAncestorの左の子に対応するカッコ開くがないかを検索します。
						ExtensionTreeNode *searched = searchedAncestor; // searchedAncestorの内部からカッコ開くを検索するためのカーソルです。
						while (searched && !searched->parenOpenBeforeClose){
							searched = searched->left;
						}
						if (searched){
							// 対応付けられていないカッコ開くを一つ削除し、ノードがカッコに囲まれていることを記録します。
							--searched->parenOpenBeforeClose;
							searchedAncestor->inParen = true;
							break;
						}
						else{
							searchedAncestor = searchedAncestor->parent;
						}
					}
					++tokenCursol;
				}


				// 演算子(オペレーターを読み込みます。
				Operator temporary_operator = Operator(NOT_TOKEN, 0 ); // 現在読み込んでいる演算子の情報です。

				// 現在見ている演算子の情報を探します。
				found = false;
				for (int j = 0; j < sizeof(OPERATOR_LIST) / sizeof(OPERATOR_LIST[0]); ++j){
					if (OPERATOR_LIST[j].kind_ == tokenCursol->kind){
						temporary_operator = OPERATOR_LIST[j];
						found = true;
						break;
					}
				}
				if (found)
				{
					// 見つかった演算子の情報をもとにノードを入れ替えます。
					ExtensionTreeNode *tmp = currentNode; //ノードを入れ替えるために使う変数です。

					ExtensionTreeNode *searched = tmp; // 入れ替えるノードを探すためのカーソルです。

					//カッコにくくられていなかった場合に、演算子の優先順位を参考に結合するノードを探します。
					bool first = true; // 演算子の優先順位を検索する最初のループです。
					do{
						if (!first){
							tmp = tmp->parent;
							searched = tmp;
						}
						// 現在の読み込み場所をくくるカッコが開く場所を探します。
						while (searched && !searched->parenOpenBeforeClose){
							searched = searched->left;
						}
						first = false;
					} while (!searched && tmp->parent && (tmp->parent->middle_operator.order_ <= temporary_operator.order_ || tmp->parent->inParen));

					// 演算子のノードを新しく生成します。
					if (MAX_EXTENSION_TREE_NODE_COUNT <= whereExtensionNodesNum){
						error = ERR_MEMORY_OVER;
						goto ERROR;
					}
					currentNode = &whereExtensionNodes[whereExtensionNodesNum++];
					currentNode->middle_operator = temporary_operator;

					// 見つかった場所に新しいノードを配置します。これまでその位置にあったノードは左の子となるよう、親ノードと子ノードのポインタをつけかえます。
					currentNode->parent = tmp->parent;
					if (currentNode->parent){
						currentNode->parent->right = currentNode;
					}
					currentNode->left = tmp;
					tmp->parent = currentNode;

					++tokenCursol;
				}
				else{
					// 現在見ている種類が演算子の一覧から見つからなければ、WHERE句は終わります。
					break;
				}
			}

			// 木を根に向かってさかのぼり、根のノードを設定します。
			whereTopNode = currentNode;
			while (whereTopNode->parent){
				whereTopNode = whereTopNode->parent;
			}
		}
	}

	// FROM句を読み込みます。
	if (tokenCursol->kind == FROM){
		++tokenCursol;
	}
	else{
		error = ERR_SQL_SYNTAX;
		goto ERROR;
	}
	bool first = true; // FROM句の最初のテーブル名を読み込み中かどうかです。
	while (tokenCursol->kind == COMMA || first){
		if (tokenCursol->kind == COMMA){
			++tokenCursol;
		}
		if (tokenCursol->kind == IDENTIFIER){
			if (MAX_TABLE_COUNT <= tableNamesNum){
				error = ERR_MEMORY_OVER;
				goto ERROR;
			}
			strncpy(tableNames[tableNamesNum++], tokenCursol->word, MAX_WORD_LENGTH);
			++tokenCursol;
		}
		else{
			error = ERR_SQL_SYNTAX;
			goto ERROR;
		}
		first = false;
	}

	// 最後のトークンまで読み込みが進んでいなかったらエラーです。
	if (tokenCursol < &tokens[tokensNum]){
		error = ERR_SQL_SYNTAX;
		goto ERROR;
	}
	Column inputColumns[MAX_TABLE_COUNT][MAX_COLUMN_COUNT]; // 入力されたCSVの行の情報です。
	// inputColumnsを初期化します。
	for (size_t i = 0; i < sizeof(inputColumns) / sizeof(inputColumns[0]); i++)
	{
		for (size_t j = 0; j < sizeof(inputColumns[0]) / sizeof(inputColumns[0][0]); j++)
		{
			inputColumns[i][j] = { "",  "" };
		}
	}
	int inputColumnNums[MAX_TABLE_COUNT] = { 0 }; // 各テーブルごとの列の数です。

	for (int i = 0; i < tableNamesNum; ++i){

		// 入力ファイル名を生成します。
		const char csvExtension[] = ".csv"; // csvの拡張子です。
		char fileName[MAX_WORD_LENGTH + sizeof(csvExtension) - 1] = ""; // 拡張子を含む、入力ファイルのファイル名です。
		strncat(fileName, tableNames[i], MAX_WORD_LENGTH + sizeof(csvExtension) - 1);
		strncat(fileName, csvExtension, MAX_WORD_LENGTH + sizeof(csvExtension) - 1);

		// 入力ファイルを開きます。
		input_table_files[i] = fopen(fileName, "r");
		if (!input_table_files[i]){
			error = ERR_FILE_OPEN;
			goto ERROR;
		}

		// 入力CSVのヘッダ行を読み込みます。
		char inputLine[MAX_FILE_LINE_LENGTH] = ""; // ファイルから読み込んだ行文字列です。
		if (fgets(inputLine, MAX_FILE_LINE_LENGTH, input_table_files[i])){
			charactorCursol = inputLine;

			// 読み込んだ行を最後まで読みます。
			while (*charactorCursol && *charactorCursol != '\r' && *charactorCursol != '\n'){
				if (MAX_COLUMN_COUNT <= inputColumnNums[i]){
					error = ERR_MEMORY_OVER;
					goto ERROR;
				}
				strncpy(inputColumns[i][inputColumnNums[i]].table_name, tableNames[i], MAX_WORD_LENGTH);
				char *writeCursol = inputColumns[i][inputColumnNums[i]++].column_name; // 列名の書き込みに利用するカーソルです。

				// 列名を一つ読みます。
				while (*charactorCursol && *charactorCursol != ',' && *charactorCursol != '\r'&& *charactorCursol != '\n'){
					*writeCursol++ = *charactorCursol++;
				}
				// 書き込んでいる列名の文字列に終端文字を書き込みます。
				writeCursol[1] = '\0';

				// 入力行のカンマの分を読み進めます。
				++charactorCursol;
			}
		}
		else{
			error = ERR_CSV_SYNTAX;
			goto ERROR;
		}

		// 入力CSVのデータ行を読み込みます。
		int rowNum = 0;
		while (fgets(inputLine, MAX_FILE_LINE_LENGTH, input_table_files[i])){
			if (MAX_ROW_COUNT <= rowNum){
				error = ERR_MEMORY_OVER;
				goto ERROR;
			}
			Data **row = input_data[i][rowNum++] = (Data**)malloc(MAX_COLUMN_COUNT * sizeof(Data*)); // 入力されている一行分のデータです。
			if (!row){
				error = ERR_MEMORY_ALLOCATE;
				goto ERROR;
			}
			// 生成した行を初期化します。
			for (int j = 0; j < MAX_COLUMN_COUNT; ++j){
				row[j] = nullptr;
			}

			charactorCursol = inputLine;
			int columnNum = 0; // いま何列目を読み込んでいるか。0基底の数字となります。

			// 読み込んだ行を最後まで読みます。
			while (*charactorCursol && *charactorCursol != '\r'&& *charactorCursol != '\n'){

				// 読み込んだデータを書き込む行のカラムを生成します。
				if (MAX_COLUMN_COUNT <= columnNum){
					error = ERR_MEMORY_OVER;
					goto ERROR;
				}
				row[columnNum] = (Data*)malloc(sizeof(Data));
				if (!row[columnNum]){
					error = ERR_MEMORY_ALLOCATE;
					goto ERROR;
				}
				*row[columnNum] =Data("");
				char *writeCursol = row[columnNum++]->string_data; // データ文字列の書き込みに利用するカーソルです。

				// データ文字列を一つ読みます。
				while (*charactorCursol && *charactorCursol != ',' && *charactorCursol != '\r'&& *charactorCursol != '\n'){
					*writeCursol++ = *charactorCursol++;
				}
				// 書き込んでいる列名の文字列に終端文字を書き込みます。
				writeCursol[1] = '\0';

				// 入力行のカンマの分を読み進めます。
				++charactorCursol;
			}
		}

		// 全てが数値となる列は数値列に変換します。
		for (int j = 0; j < inputColumnNums[i]; ++j){

			// 全ての行のある列について、データ文字列から符号と数値以外の文字を探します。
			current_row = input_data[i];
			found = false;
			while (*current_row){
				char *currentChar = (*current_row)[j]->string_data;
				while (*currentChar){
					bool isNum = false;
					const char *currentNum = SIGNED_AND_NUMBER;
					while (*currentNum){
						if (*currentChar == *currentNum){
							isNum = true;
							break;
						}
						++currentNum;
					}
					if (!isNum){
						found = true;
						break;
					}
					++currentChar;
				}
				if (found){
					break;
				}
				++current_row;
			}

			// 符号と数字以外が見つからない列については、数値列に変換します。
			if (!found){
				current_row = input_data[i];
				while (*current_row){
					*(*current_row)[j] = Data(atoi((*current_row)[j]->string_data));
					++current_row;
				}
			}
		}
	}

	Column allInputColumns[MAX_TABLE_COUNT * MAX_COLUMN_COUNT]; // 入力に含まれるすべての列の一覧です。
	// allInputColumnsを初期化します。
	for (size_t i = 0; i < sizeof(allInputColumns) / sizeof(allInputColumns[0]); i++)
	{
		allInputColumns[i] = { "", "" };
	}
	int allInputColumnsNum = 0; // 入力に含まれるすべての列の数です。

	// 入力ファイルに書いてあったすべての列をallInputColumnsに設定します。
	for (int i = 0; i < tableNamesNum; ++i){
		for (int j = 0; j < inputColumnNums[i]; ++j){
			strncpy(allInputColumns[allInputColumnsNum].table_name, tableNames[i], MAX_WORD_LENGTH);
			strncpy(allInputColumns[allInputColumnsNum++].column_name, inputColumns[i][j].column_name, MAX_WORD_LENGTH);
		}
	}

	// SELECT句の列名指定が*だった場合は、入力CSVの列名がすべて選択されます。
	if (!selectColumnsNum){
		for (int i = 0; i < allInputColumnsNum; ++i){
			strncpy(selectColumns[selectColumnsNum].table_name, allInputColumns[i].table_name, MAX_WORD_LENGTH);
			strncpy(selectColumns[selectColumnsNum++].column_name, allInputColumns[i].column_name, MAX_WORD_LENGTH);
		}
	}

	Column outputColumns[MAX_TABLE_COUNT * MAX_COLUMN_COUNT]; // 出力するすべての行の情報です。
	int outputColumnNum = 0; // 出力するすべての行の現在の数です。

	// SELECT句で指定された列名が、何個目の入力ファイルの何列目に相当するかを判別します。
	ColumnIndex selectColumnIndexes[MAX_TABLE_COUNT * MAX_COLUMN_COUNT]; // SELECT句で指定された列の、入力ファイルとしてのインデックスです。
	int selectColumnIndexesNum = 0; // selectColumnIndexesの現在の数。
	for (int i = 0; i < selectColumnsNum; ++i){
		found = false;
		for (int j = 0; j < tableNamesNum; ++j){
			for (int k = 0; k < inputColumnNums[j]; ++k){
				char* selectTableNameCursol = selectColumns[i].table_name;
				char* inputTableNameCursol = inputColumns[j][k].table_name;
				while (*selectTableNameCursol && toupper(*selectTableNameCursol) == toupper(*inputTableNameCursol++)){
					++selectTableNameCursol;
				}
				char* selectColumnNameCursol = selectColumns[i].column_name;
				char* inputColumnNameCursol = inputColumns[j][k].column_name;
				while (*selectColumnNameCursol && toupper(*selectColumnNameCursol) == toupper(*inputColumnNameCursol++)){
					++selectColumnNameCursol;
				}
				if (!*selectColumnNameCursol && !*inputColumnNameCursol &&
					(!*selectColumns[i].table_name || // テーブル名が設定されている場合のみテーブル名の比較を行います。
					!*selectTableNameCursol && !*inputTableNameCursol)){

					// 既に見つかっているのにもう一つ見つかったらエラーです。
					if (found){
						error = ERR_BAD_COLUMN_NAME;
						goto ERROR;
					}
					found = true;
					// 見つかった値を持つ列のデータを生成します。
					if (MAX_COLUMN_COUNT <= selectColumnIndexesNum){
						error = ERR_MEMORY_OVER;
						goto ERROR;
					}
					selectColumnIndexes[selectColumnIndexesNum++] = { j, k };
				}
			}
		}
		// 一つも見つからなくてもエラーです。
		if (!found){
			error = ERR_BAD_COLUMN_NAME;
			goto ERROR;
		}
	}

	// 出力する列名を設定します。
	for (int i = 0; i < selectColumnsNum; ++i){
		strncpy(outputColumns[outputColumnNum].table_name, inputColumns[selectColumnIndexes[i].table][selectColumnIndexes[i].column].table_name, MAX_WORD_LENGTH);
		strncpy(outputColumns[outputColumnNum].column_name, inputColumns[selectColumnIndexes[i].table][selectColumnIndexes[i].column].column_name, MAX_WORD_LENGTH);
		++outputColumnNum;
	}

	if (whereTopNode){
		// 既存数値の符号を計算します。
		for (int i = 0; i < whereExtensionNodesNum; ++i){
			if (whereExtensionNodes[i].middle_operator.kind_ == NOT_TOKEN &&
				!*whereExtensionNodes[i].column.column_name &&
				whereExtensionNodes[i].value.type == INTEGER){
				whereExtensionNodes[i].value.integer *= whereExtensionNodes[i].signCoefficient;
			}
		}
	}

	int outputRowsNum = 0; // 出力データの現在の行数です。

	Data ***currentRows[MAX_TABLE_COUNT] = { nullptr }; // 入力された各テーブルの、現在出力している行を指すカーソルです。
	for (int i = 0; i < tableNamesNum; ++i){
		// 各テーブルの先頭行を設定します。
		currentRows[i] = input_data[i];
	}

	// 出力するデータを設定します。
	while (true){
		if (MAX_ROW_COUNT <= outputRowsNum){
			error = ERR_MEMORY_OVER;
			goto ERROR;
		}
		Data **row = output_data[outputRowsNum] = (Data**)malloc(MAX_COLUMN_COUNT * sizeof(Data*)); // 出力している一行分のデータです。
		if (!row){
			error = ERR_MEMORY_ALLOCATE;
			goto ERROR;
		}

		// 生成した行を初期化します。
		for (int i = 0; i < MAX_COLUMN_COUNT; ++i){
			row[i] = nullptr;
		}

		// 行の各列のデータを入力から持ってきて設定します。
		for (int i = 0; i < selectColumnIndexesNum; ++i){
			row[i] = (Data*)malloc(sizeof(Data));
			if (!row[i]){
				error = ERR_MEMORY_ALLOCATE;
				goto ERROR;
			}
			*row[i] = *(*currentRows[selectColumnIndexes[i].table])[selectColumnIndexes[i].column];
		}

		Data **allColumnsRow = all_column_output_data[outputRowsNum++] = (Data**)malloc(MAX_TABLE_COUNT * MAX_COLUMN_COUNT * sizeof(Data*)); // WHEREやORDERのためにすべての情報を含む行。rowとインデックスを共有します。
		if (!allColumnsRow){
			error = ERR_MEMORY_ALLOCATE;
			goto ERROR;
		}
		// 生成した行を初期化します。
		for (int i = 0; i < MAX_TABLE_COUNT * MAX_COLUMN_COUNT; ++i){
			allColumnsRow[i] = nullptr;
		}

		// allColumnsRowの列を設定します。
		int allColumnsNum = 0; // allColumnsRowの現在の列数です。
		for (int i = 0; i < tableNamesNum; ++i){
			for (int j = 0; j < inputColumnNums[i]; ++j){
				allColumnsRow[allColumnsNum] = (Data*)malloc(sizeof(Data));
				if (!allColumnsRow[allColumnsNum]){
					error = ERR_MEMORY_ALLOCATE;
					goto ERROR;
				}
				*allColumnsRow[allColumnsNum++] = *(*currentRows[i])[j];
			}
		}
		// WHEREの条件となる値を再帰的に計算します。
		if (whereTopNode){
			ExtensionTreeNode *currentNode = whereTopNode; // 現在見ているノードです。
			while (currentNode){
				// 子ノードの計算が終わってない場合は、まずそちらの計算を行います。
				if (currentNode->left && !currentNode->left->calculated){
					currentNode = currentNode->left;
					continue;
				}
				else if (currentNode->right && !currentNode->right->calculated){
					currentNode = currentNode->right;
					continue;
				}

				// 自ノードの値を計算します。
				switch (currentNode->middle_operator.kind_){
				case NOT_TOKEN:
					// ノードにデータが設定されている場合です。

					// データが列名で指定されている場合、今扱っている行のデータを設定します。
					if (*currentNode->column.column_name){
						found = false;
						for (int i = 0; i < allInputColumnsNum; ++i){
							char* whereTableNameCursol = currentNode->column.table_name;
							char* allInputTableNameCursol = allInputColumns[i].table_name;
							while (*whereTableNameCursol && toupper(*whereTableNameCursol) == toupper(*allInputTableNameCursol++)){
								++whereTableNameCursol;
							}
							char* whereColumnNameCursol = currentNode->column.column_name;
							char* allInputColumnNameCursol = allInputColumns[i].column_name;
							while (*whereColumnNameCursol && toupper(*whereColumnNameCursol) == toupper(*allInputColumnNameCursol++)){
								++whereColumnNameCursol;
							}
							if (!*whereColumnNameCursol && !*allInputColumnNameCursol &&
								(!*currentNode->column.table_name || // テーブル名が設定されている場合のみテーブル名の比較を行います。
								!*whereTableNameCursol && !*allInputTableNameCursol)){
								// 既に見つかっているのにもう一つ見つかったらエラーです。
								if (found){
									error = ERR_BAD_COLUMN_NAME;
									goto ERROR;
								}
								found = true;
								currentNode->value = *allColumnsRow[i];
							}
						}
						// 一つも見つからなくてもエラーです。
						if (!found){
							error = ERR_BAD_COLUMN_NAME;
							goto ERROR;
						}
						;
						// 符号を考慮して値を計算します。
						if (currentNode->value.type == INTEGER){
							currentNode->value.integer *= currentNode->signCoefficient;
						}
					}
					break;
				case EQUAL:
				case GREATER_THAN:
				case GREATER_THAN_OR_EQUAL:
				case LESS_THAN:
				case LESS_THAN_OR_EQUAL:
				case NOT_EQUAL:
					// 比較演算子の場合です。

					// 比較できるのは文字列型か整数型で、かつ左右の型が同じ場合です。
					if (currentNode->left->value.type != INTEGER && currentNode->left->value.type != STRING ||
						currentNode->left->value.type != currentNode->right->value.type){
						error = ERR_WHERE_OPERAND_TYPE;
						goto ERROR;
					}
					currentNode->value.type = BOOLEAN;

					// 比較結果を型と演算子によって計算方法を変えて、計算します。
					switch (currentNode->left->value.type){
					case INTEGER:
						switch (currentNode->middle_operator.kind_){
						case EQUAL:
							currentNode->value.boolean = currentNode->left->value.integer == currentNode->right->value.integer;
							break;
						case GREATER_THAN:
							currentNode->value.boolean = currentNode->left->value.integer > currentNode->right->value.integer;
							break;
						case GREATER_THAN_OR_EQUAL:
							currentNode->value.boolean = currentNode->left->value.integer >= currentNode->right->value.integer;
							break;
						case LESS_THAN:
							currentNode->value.boolean = currentNode->left->value.integer < currentNode->right->value.integer;
							break;
						case LESS_THAN_OR_EQUAL:
							currentNode->value.boolean = currentNode->left->value.integer <= currentNode->right->value.integer;
							break;
						case NOT_EQUAL:
							currentNode->value.boolean = currentNode->left->value.integer != currentNode->right->value.integer;
							break;
						}
						break;
					case STRING:
						switch (currentNode->middle_operator.kind_){
						case EQUAL:
							currentNode->value.boolean = strcmp(currentNode->left->value.string_data, currentNode->right->value.string_data) == 0;
							break;
						case GREATER_THAN:
							currentNode->value.boolean = strcmp(currentNode->left->value.string_data, currentNode->right->value.string_data) > 0;
							break;
						case GREATER_THAN_OR_EQUAL:
							currentNode->value.boolean = strcmp(currentNode->left->value.string_data, currentNode->right->value.string_data) >= 0;
							break;
						case LESS_THAN:
							currentNode->value.boolean = strcmp(currentNode->left->value.string_data, currentNode->right->value.string_data) < 0;
							break;
						case LESS_THAN_OR_EQUAL:
							currentNode->value.boolean = strcmp(currentNode->left->value.string_data, currentNode->right->value.string_data) <= 0;
							break;
						case NOT_EQUAL:
							currentNode->value.boolean = strcmp(currentNode->left->value.string_data, currentNode->right->value.string_data) != 0;
							break;
						}
						break;
					}
					break;
				case PLUS:
				case MINUS:
				case ASTERISK:
				case SLASH:
					// 四則演算の場合です。

					// 演算できるのは整数型同士の場合のみです。
					if (currentNode->left->value.type != INTEGER || currentNode->right->value.type != INTEGER){
						error = ERR_WHERE_OPERAND_TYPE;
						goto ERROR;
					}
					currentNode->value.type = INTEGER;

					// 比較結果を演算子によって計算方法を変えて、計算します。
					switch (currentNode->middle_operator.kind_){
					case PLUS:
						currentNode->value.integer = currentNode->left->value.integer + currentNode->right->value.integer;
						break;
					case MINUS:
						currentNode->value.integer = currentNode->left->value.integer - currentNode->right->value.integer;
						break;
					case ASTERISK:
						currentNode->value.integer = currentNode->left->value.integer * currentNode->right->value.integer;
						break;
					case SLASH:
						currentNode->value.integer = currentNode->left->value.integer / currentNode->right->value.integer;
						break;
					}
					break;
				case AND:
				case OR:
					// 論理演算の場合です。

					// 演算できるのは真偽値型同士の場合のみです。
					if (currentNode->left->value.type != BOOLEAN || currentNode->right->value.type != BOOLEAN){
						error = ERR_WHERE_OPERAND_TYPE;
						goto ERROR;
					}
					currentNode->value.type = BOOLEAN;

					// 比較結果を演算子によって計算方法を変えて、計算します。
					switch (currentNode->middle_operator.kind_){
					case AND:
						currentNode->value.boolean = currentNode->left->value.boolean && currentNode->right->value.boolean;
						break;
					case OR:
						currentNode->value.boolean = currentNode->left->value.boolean || currentNode->right->value.boolean;
						break;
					}
				}
				currentNode->calculated = true;

				// 自身の計算が終わった後は親の計算に戻ります。
				currentNode = currentNode->parent;
			}

			// 条件に合わない行は出力から削除します。
			if (!whereTopNode->value.boolean){
				free(row);
				free(allColumnsRow);
				all_column_output_data[--outputRowsNum] = nullptr;
				output_data[outputRowsNum] = nullptr;
			}
			// WHERE条件の計算結果をリセットします。
			for (int i = 0; i < whereExtensionNodesNum; ++i){
				whereExtensionNodes[i].calculated = false;
			}
		}

		// 各テーブルの行のすべての組み合わせを出力します。

		// 最後のテーブルのカレント行をインクリメントします。
		++currentRows[tableNamesNum - 1];

		// 最後のテーブルが最終行になっていた場合は先頭に戻し、順に前のテーブルのカレント行をインクリメントします。
		for (int i = tableNamesNum - 1; !*currentRows[i] && 0 < i; --i){
			++currentRows[i - 1];
			currentRows[i] = input_data[i];
		}

		// 最初のテーブルが最後の行を超えたなら出力行の生成は終わりです。
		if (!*currentRows[0]){
			break;
		}
	}

	// ORDER句による並び替えの処理を行います。
	if (orderByColumnsNum){
		// ORDER句で指定されている列が、全ての入力行の中のどの行なのかを計算します。
		int orderByColumnIndexes[MAX_COLUMN_COUNT]; // ORDER句で指定された列の、すべての行の中でのインデックスです。
		int orderByColumnIndexesNum = 0; // 現在のorderByColumnIndexesの数です。
		for (int i = 0; i < orderByColumnsNum; ++i){
			found = false;
			for (int j = 0; j < allInputColumnsNum; ++j){
				char* orderByTableNameCursol = orderByColumns[i].table_name;
				char* allInputTableNameCursol = allInputColumns[j].table_name;
				while (*orderByTableNameCursol && toupper(*orderByTableNameCursol) == toupper(*allInputTableNameCursol)){
					++orderByTableNameCursol;
					++allInputTableNameCursol;
				}
				char* orderByColumnNameCursol = orderByColumns[i].column_name;
				char* allInputColumnNameCursol = allInputColumns[j].column_name;
				while (*orderByColumnNameCursol && toupper(*orderByColumnNameCursol) == toupper(*allInputColumnNameCursol)){
					++orderByColumnNameCursol;
					++allInputColumnNameCursol;
				}
				if (!*orderByColumnNameCursol && !*allInputColumnNameCursol &&
					(!*orderByColumns[i].table_name || // テーブル名が設定されている場合のみテーブル名の比較を行います。
					!*orderByTableNameCursol && !*allInputTableNameCursol)){
					// 既に見つかっているのにもう一つ見つかったらエラーです。
					if (found){
						error = ERR_BAD_COLUMN_NAME;
						goto ERROR;
					}
					found = true;
					if (MAX_COLUMN_COUNT <= orderByColumnIndexesNum){
						error = ERR_MEMORY_OVER;
						goto ERROR;
					}
					orderByColumnIndexes[orderByColumnIndexesNum++] = j;
				}
			}
			// 一つも見つからなくてもエラーです。
			if (!found){
				error = ERR_BAD_COLUMN_NAME;
				goto ERROR;
			}
		}

		// outputDataとallColumnOutputDataのソートを一緒に行います。簡便のため凝ったソートは使わず、選択ソートを利用します。
		for (int i = 0; i < outputRowsNum; ++i){
			int minIndex = i; // 現在までで最小の行のインデックスです。
			for (int j = i + 1; j < outputRowsNum; ++j){
				bool jLessThanMin = false; // インデックスがjの値が、minIndexの値より小さいかどうかです。
				for (int k = 0; k < orderByColumnIndexesNum; ++k){
					Data *mData = all_column_output_data[minIndex][orderByColumnIndexes[k]]; // インデックスがminIndexのデータです。
					Data *jData = all_column_output_data[j][orderByColumnIndexes[k]]; // インデックスがjのデータです。
					int cmp = 0; // 比較結果です。等しければ0、インデックスjの行が大きければプラス、インデックスminIndexの行が大きければマイナスとなります。
					switch (mData->type)
					{
					case INTEGER:
						cmp = jData->integer - mData->integer;
						break;
					case STRING:
						cmp = strcmp(jData->string_data, mData->string_data);
						break;
					}

					// 降順ならcmpの大小を入れ替えます。
					if (orders[k] == DESC){
						cmp *= -1;
					}
					if (cmp < 0){
						jLessThanMin = true;
						break;
					}
					else if (0 < cmp){
						break;
					}
				}
				if (jLessThanMin){
					minIndex = j;
				}
			}
			Data** tmp = output_data[minIndex];
			output_data[minIndex] = output_data[i];
			output_data[i] = tmp;

			tmp = all_column_output_data[minIndex];
			all_column_output_data[minIndex] = all_column_output_data[i];
			all_column_output_data[i] = tmp;
		}
	}

	// 出力ファイルを開きます。
	output_file = fopen(output_file_name, "w");
	if (output_file == nullptr){
		error = ERR_FILE_OPEN;
		goto ERROR;
	}

	// 出力ファイルに列名を出力します。
	for (int i = 0; i < selectColumnsNum; ++i){
		result = fputs(outputColumns[i].column_name, output_file);
		if (result == EOF){
			error = ERR_FILE_WRITE;
			goto ERROR;
		}
		if (i < selectColumnsNum - 1){
			result = fputs(",", output_file);
			if (result == EOF){
				error = ERR_FILE_WRITE;
				goto ERROR;
			}
		}
		else{
			result = fputs("\n", output_file);
			if (result == EOF){
				error = ERR_FILE_WRITE;
				goto ERROR;
			}
		}
	}

	// 出力ファイルにデータを出力します。
	current_row = output_data;
	while (*current_row){
		Data **column = *current_row;
		for (int i = 0; i < selectColumnsNum; ++i){
			char outputString[MAX_DATA_LENGTH] = "";
			switch ((*column)->type){
			case INTEGER:
				itoa((*column)->integer, outputString, 10);
				break;
			case STRING:
				strcpy(outputString, (*column)->string_data);
				break;
			}
			result = fputs(outputString, output_file);
			if (result == EOF){
				error = ERR_FILE_WRITE;
				goto ERROR;
			}
			if (i < selectColumnsNum - 1){
				result = fputs(",", output_file);
				if (result == EOF){
					error = ERR_FILE_WRITE;
					goto ERROR;
				}
			}
			else{
				result = fputs("\n", output_file);
				if (result == EOF){
					error = ERR_FILE_WRITE;
					goto ERROR;
				}
			}
			++column;
		}
		++current_row;
	}

	// 正常時の後処理です。

	// ファイルリソースを解放します。
	for (int i = 0; i < MAX_TABLE_COUNT; ++i){
		if (input_table_files[i]){
			fclose(input_table_files[i]);
			if (result == EOF){
				error = ERR_FILE_CLOSE;
				goto ERROR;
			}
		}
	}
	if (output_file){
		fclose(output_file);
		if (result == EOF){
			error = ERR_FILE_CLOSE;
			goto ERROR;
		}
	}

	// メモリリソースを解放します。
	for (int i = 0; i < tableNamesNum; ++i){
		current_row = input_data[i];
		while (*current_row){
			Data **dataCursol = *current_row;
			while (*dataCursol){
				free(*dataCursol++);
			}
			free(*current_row);
			current_row++;
		}
	}
	current_row = output_data;
	while (*current_row){
		Data **dataCursol = *current_row;
		while (*dataCursol){
			free(*dataCursol++);
		}
		free(*current_row);
		current_row++;
	}
	current_row = all_column_output_data;
	while (*current_row){
		Data **dataCursol = *current_row;
		while (*dataCursol){
			free(*dataCursol++);
		}
		free(*current_row);
		current_row++;
	}

	return OK;

ERROR:
	// エラー時の処理です。

	// ファイルリソースを解放します。
	for (int i = 0; i < MAX_TABLE_COUNT; ++i){
		if (input_table_files[i]){
			fclose(input_table_files[i]);
		}
	}
	if (output_file){
		fclose(output_file);
	}

	// メモリリソースを解放します。
	for (int i = 0; i < tableNamesNum; ++i){
		current_row = input_data[i];
		while (*current_row){
			Data **dataCursol = *current_row;
			while (*dataCursol){
				free(*dataCursol++);
			}
			free(*current_row);
			current_row++;
		}
	}
	current_row = output_data;
	while (*current_row){
		Data **dataCursol = *current_row;
		while (*dataCursol){
			free(*dataCursol++);
		}
		free(*current_row);
		current_row++;
	}
	current_row = all_column_output_data;
	while (*current_row){
		Data **dataCursol = *current_row;
		while (*dataCursol){
			free(*dataCursol++);
		}
		free(*current_row);
		current_row++;
	}
	return  error;
}