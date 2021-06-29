#include <vector>
#include <string>
#include <fstream>
#include <boost/assert.hpp>
#include <cctype>
#include <regex>
#include <iostream>
using namespace std;
#include "ctoken.h"
#include "clexer.h"

static int read_number(string& str, int n);

CLexer::CLexer(CFileInfo &infile) : infile(infile)
{
}

vector<CToken>& CLexer::getTokens()
{
	int line_no = 0;
	for (string& line : infile.lines) {
		line_no++;
		for (int n = 0; n < line.size(); ) {
			char c = line[n];
			if (isspace(c)) {
				n++;
				continue;
			}

			if (isdigit(c) || c == '.' && isdigit(line[n+1])) {
				int len = read_number(line, n);
				n += len;
				continue;
			}

			n++;
			// BOOST_ASSERT(false);
		}
	}

	return tokens;
}

int read_number(string& str, int n)
{
	int len = 1;
	regex digit_re("^[0-9]+");

	cmatch m;
	const char* target_str = str.c_str() + n;
	regex_search(target_str,  m, digit_re);
	BOOST_ASSERT(m.size() == 1);

	len = m.length(0);

	return len;
}
