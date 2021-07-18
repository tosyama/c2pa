#include <vector>
#include <string>
#include <boost/assert.hpp>
using namespace std;
#include "ctoken.h"

CToken0::CToken0(CToken0Type type, int line_no, int pos, int len)
	: type(type), is_eol(false)
{
	this->line_no = line_no;
	this->pos = pos;
	this->len = len;
}

CToken::CToken(CTokenType type, int lexer_no, int start_token_no, int end_token_no)
	: type(type), lexer_no(lexer_no),
		start_token_no(start_token_no), end_token_no(end_token_no)
{
	if (type == TT_INCLUDE) {
		info.tokens = new vector<CToken*>();
	}
}

CToken::~CToken()
{
	if (type == TT_INCLUDE) {
		delete info.tokens;
	}
}
