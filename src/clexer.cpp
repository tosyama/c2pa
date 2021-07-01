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
static int read_id(string& str, int n);

CLexer::CLexer(CFileInfo &infile) : infile(infile)
{
}

vector<CToken>& CLexer::getTokens()
{
	int line_no = 0;
	bool in_comment = false;
	bool in_str_literal = false;
	int start_pos;	// for comment, string literal

	for (string& line : infile.lines) {
		// use for macro detection
		int token_num = 0;	// in a line. comment is not counted.
		bool in_brace_str = false;
		bool macro_mode = false;
		bool include_file_mode = false;

		line_no++;
		start_pos = 0;
		token_num = 0;
		for (int n = 0; n < line.size(); ) {
			char c0 = line[n];
			char c1 = (n+1 < line.size()) ? line[n+1] : '\0';

			if (in_comment) {	// Comment
				if (c0 == '*') {
					if (c1 == '/') {	// Block comment end
						in_comment = false;
						n += 2;
						int len = n - start_pos;
						tokens.emplace_back(TT_COMMENT, line_no, start_pos, len);
						continue;
					}
				}
				n++;
				continue;
			}

			if (in_str_literal) {	// String literal
				if (c0 == '\\' && c1) {
					n += 2;
					continue;
				}

				if (c0 == '\"') {
					in_str_literal = false;
					n++;
					int len = n - start_pos;
					token_num++;
					tokens.emplace_back(TT_STR, line_no, start_pos, len);
					continue;
				}
				n++;
				continue;
			}

			// Space
			if (isspace(c0)) {
				n++;
				continue;
			}

			if (c0 == '/') {
				if (c1 == '/') {	// Line comment
					int len = line.size() - n;
					tokens.emplace_back(TT_COMMENT, line_no, n, len);
					break;
				}

				if (c1 == '*') {	// Block comment start
					in_comment = true;
					start_pos = n;
					n++;
					continue;
				}
			}

			// Number
			if (isdigit(c0) || c0 == '.' && isdigit(c1)) {
				int len = read_number(line, n);
				tokens.emplace_back(TT_NUMBER, line_no, n, len);
				n += len;
				token_num++;
				continue;
			}

			// String literal start
			if (c0 == '\"') {
				in_str_literal = true;
				start_pos = n;
				n++;
				continue;
			}

			// Identifier
			if (isalpha(c0) || c0 == '_') {
				int len = read_id(line, n);
				tokens.emplace_back(TT_ID, line_no, n, len);
				token_num++;
				if (macro_mode && token_num == 2) {
					// detect "include"
					if (len == 7) {
						const char* idstr = line.c_str() + n;
						if (!strncmp(idstr, "include", len)) {
							include_file_mode = true;
						}
					}
				}
				n += len;
				continue;
			}

			if (include_file_mode) {	// Special flow for "#include <...>"
				if (token_num == 2 && c0 == '<') {
					int s = n;
					int len = 0;
					for (; n<line.size(); n++) {
						len++;
						if (line[n] == '>') {
							n++;
							break;
						}
					}
				
					tokens.emplace_back(TT_STR, line_no, s, len);
					include_file_mode = false;
					continue;
				}
				include_file_mode = false;
			}

			int16_t c = (c0 << 8) | c1;

			// Punctuator
			{
				int len = 1;
				if (c == '<<' || c == '>>') {	// also "<<=", ">>=", "..."
					char c2 = (n+2 < line.size()) ? line[n+2] : '\0';
					len = 2;
					if (c2 == '=') {
						len = 3;
					}

				} else if (c == '==' || c == '!=' || c == '<=' || c == '>='
						|| c == '+=' || c == '-=' || c == '*=' || c == '/=' || c == '%='
						|| c == '&=' || c == '|=' || c == '^='
						|| c == '&&' || c == '||'
						|| c == '##') {
					len = 2;

				} else if (c == '..') {
					char c2 = (n+2 < line.size()) ? line[n+2] : '\0';
					if (c2 == '.') {
						len = 3;
					}
				}

				if (token_num == 0 && c0 == '#') {
					macro_mode = true;
				}

				tokens.emplace_back(TT_PUNCTUATOR, line_no, n, len);
				n += len;
				token_num++;
			}
		}

		if (in_comment) { // Block comment continues next line
			int len = line.size() - start_pos;
			tokens.emplace_back(TT_COMMENT, line_no, start_pos, len);
		}

		if (in_str_literal) { // String Literal continues next line
			int len = line.size() - start_pos;
			tokens.emplace_back(TT_STR, line_no, start_pos, len);
		}
	}

	return tokens;
}

static regex rex_numberpart("^[0-9A-Za-z\\.]+");
static regex rex_exponent("^[+-]?[0-9]{1}[0-9A-Za-z]*");

int read_number(string& str, int n)
{	// Note: this exclude number-like token include invalid format.
	cmatch m;
	const char* target_str = str.c_str() + n;
	regex_search(target_str,  m, rex_numberpart);
	BOOST_ASSERT(m.size() == 1);
	int len = m.length(0);
	BOOST_ASSERT(len > 0);

	char c = str[n+len-1];
	if ((c == 'E' || c == 'e') && (n+len)<str.size() ) {
		const char* target_str_e = str.c_str() + n + len;
		regex_search(target_str_e,  m, rex_exponent);
		if (m.size() >= 1) {
			len += m.length(0);
		}
	}

	return len;
}

static regex rex_id("^[A-Za-z_]{1}[A-Za-z0-9_]*");
int read_id(string& str, int n)
{
	cmatch m;
	const char* target_str = str.c_str() + n;
	regex_search(target_str,  m, rex_id);
	BOOST_ASSERT(m.size() == 1);
	return m.length(0);
}

