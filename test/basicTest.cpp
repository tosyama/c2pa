#include <vector>
#include <string>
#include <gtest/gtest.h>
using namespace std;
#include "../src/ctoken.h"
#include "../src/clexer.h"

TEST(HelloWorld, Open) {
	CFileInfo finfo("cases/001_helloworld.c");
	ASSERT_EQ(finfo.lines.size(), 7);

	string& line = finfo.getLine(7);
	ASSERT_EQ(line, "}");

	CLexer lexer(finfo);
	vector<CToken>& tokens = lexer.getTokens();

	ASSERT_EQ(tokens.size(), 0);
}

