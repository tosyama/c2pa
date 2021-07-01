#include <vector>
#include <string>
#include <gtest/gtest.h>
using namespace std;
#include "../src/ctoken.h"
#include "../src/clexer.h"

static void dumptokens(vector<CToken>& tokens, vector<string>& lines)
{
	for (CToken &t: tokens) {
		string s =
			t.type == TT_ID ? "id":
			t.type == TT_NUMBER ? "number":
			t.type == TT_STR ? "str":
			t.type == TT_PUNCTUATOR ? "punc":
			t.type == TT_COMMENT ? "comment":
			"unknown";

		cout << s << ":" << lines[t.line_no-1].substr(t.pos, t.len) << endl;
	}
}

TEST(TempolaryWork, Open) {
	CFileInfo finfo("cases/000_temp.c");
	CLexer lexer(finfo);
	vector<CToken>& tokens = lexer.getTokens();
//	dumptokens(tokens, finfo.lines);
}

TEST(HelloWorld, Open) {
	CFileInfo finfo("cases/001_helloworld.c");
	ASSERT_EQ(finfo.lines.size(), 7);

	string& line = finfo.getLine(7);
	ASSERT_EQ(line, "}");

	CLexer lexer(finfo);
	vector<CToken>& tokens = lexer.getTokens();

	ASSERT_EQ(tokens.size(), 17);
}

