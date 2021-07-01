#include <vector>
#include <string>
#include <fstream>
#include <boost/assert.hpp>
using namespace std;
#include "ctoken.h"

CFileInfo::CFileInfo(string fname)
	: fname(fname)
{
	ifstream infile(fname);
	BOOST_ASSERT(!infile.fail());

	string str;
	while (getline(infile, str)) {
		lines.push_back(str);
	}
}

string& CFileInfo::getLine(int n)
{
	BOOST_ASSERT(n > 0 && n <= lines.size());
	return lines[n-1];
}


CToken::CToken(CTokenType type, int line_no, int pos, int len) : type(type)
{
	this->line_no = line_no;
	this->pos = pos;
	this->len = len;
}
