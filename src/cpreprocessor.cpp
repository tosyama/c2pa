#include <string>
#include <vector>
#include <list>
#include <iostream>
#include <algorithm>
#include <boost/assert.hpp>

using namespace std;
#include "cfileinfo.h"
#include "ctoken.h"
#include "clexer.h"
#include "cpreprocessor.h"

// utilities
class IfInfo {
public:
	CToken0* start;
	bool is_fulfilled;
	bool is_else;
	IfInfo(CToken0* t): start(t), is_fulfilled(false), is_else(false) {}
};

static int process_include(CPreprocessor& cpp, CLexer& lexer, int n, vector<CToken*>* tokens);
static int process_define(CPreprocessor& cpp, CLexer& lexer, int n);
static int process_undef(CPreprocessor& cpp, CLexer& lexer, int n);
static int process_ifdef(vector<IfInfo> &ifstack, CPreprocessor& cpp, CLexer& lexer, int n, bool is_ndef = false);
static int process_else(vector<IfInfo> &ifstack, CLexer& lexer, int n);

static int next_pos(const vector<CToken0>& token0s, int n);
static string get_str(CLexer &lexer, CToken0* t);
static int get_ch(CLexer &lexer, CToken0* t);

//== CMacro ==
CMacro::CMacro(CMacroType type, string name)
	: type(type), name(name)
{
}

//== CPreprocessor ==
bool CPreprocessor::preprocess(const string& filepath, vector<CToken*> *tokens)
{
	if (!tokens) {
		tokens = &top_tokens;
	}
	int lexer_no = lexers.size();
	CFileInfo finfo(filepath);
	lexers.push_back(new CLexer(finfo));
	CLexer& lexer = *lexers.back();
	lexer.no = lexer_no;

	vector<IfInfo> ifstack;

	vector<CToken0>& token0s = lexer.scan();

	for (int n=0; n<token0s.size(); n++) {
		CToken0 *t = &token0s[n];
		if (t->type == TT0_MACRO) {		// '#'
			if (t->is_eol) continue;	// null directive

			n = next_pos(token0s, n);
			BOOST_ASSERT(t != &token0s[n]);
			t = &token0s[n];

			if (t->type == TT0_ID) {
				string directive = get_str(lexer, t);
				if (directive == "include") {
					n = process_include(*this, lexer, n, tokens); 
					
				} else if (directive == "include_next") {

				} else if (directive == "define") {
					n = process_define(*this, lexer, n);

				} else if (directive == "undef") {
					n = process_undef(*this, lexer, n);

				} else if (directive == "if") {
				} else if (directive == "ifdef") {
					n = process_ifdef(ifstack, *this, lexer, n);

				} else if (directive == "ifndef") {
					n = process_ifdef(ifstack, *this, lexer, n, true);

				} else if (directive == "elif") {
				} else if (directive == "else") {
					n = process_else(ifstack, lexer, n);

				} else if (directive == "endif") {
				} else if (directive == "line") {
				} else if (directive == "pragma") {
				} else if (directive == "error") {
				}

			} else if (t->type == TT0_NUMBER) {

			} else {
				BOOST_ASSERT(false);	// error
			}
		}
	}
	return true;
}

//== utilities ==
void is_not_eol(CToken0& t)
{
	if (t.is_eol) {
		BOOST_ASSERT(false);
	}
}

int process_include(CPreprocessor& cpp, CLexer& lexer, int n, vector<CToken*>* tokens)
{
	vector<CToken0> &token0s = lexer.tokens;
	is_not_eol(token0s[n]);

	int start = n;
	n = next_pos(token0s, n);
	CToken0 &fname_t = token0s[n];

	if (fname_t.type != TT0_STR) {	// filename should be <...> or "..."
		BOOST_ASSERT(false);
	}

	string inc_path = get_str(lexer, &fname_t);
	string foundpath = CFileInfo::getFilePath(inc_path, lexer.infile.fname, cpp.include_paths);

	tokens->push_back(new CToken(TT_INCLUDE, lexer.no, start, n));
	if (cpp.lexers.size() < 2)
		cpp.preprocess(foundpath, tokens->back()->info.tokens);
	
	if (!fname_t.is_eol) {
		BOOST_ASSERT(false);
	}

	return n;
}

static void delete_macro(vector<CMacro*> &macros, string &macro_name);
static int parse_args(vector<string>& args, CLexer& lexer, int n);
static int set_macro_body(vector<CToken*> &body, CLexer& lexer, int n);

int process_define(CPreprocessor& cpp, CLexer& lexer, int n)
{
	CFileInfo &curfile = lexer.infile;
	vector<CToken0> &token0s = lexer.tokens;
	is_not_eol(token0s[n]);

	n = next_pos(token0s, n);
	CToken0 &t = token0s[n];

	if (t.type != TT0_ID) {
		BOOST_ASSERT(false);
	}
	string macro_name = get_str(lexer, &t);

	delete_macro(cpp.macros, macro_name);

	if (t.is_eol) {	// define only name
		cpp.macros.push_back(new CMacro(MT_ID, get_str(lexer, &t)));
		// cout << "Macro: " << cpp.macros.back()->name << endl;
		return n;
	}

	n = next_pos(token0s, n);
	CToken0 &t1 = token0s[n];

	if (t1.type == TT0_COMMENT) {	// define only name
		BOOST_ASSERT(t1.is_eol);
		cpp.macros.push_back(new CMacro(MT_ID, macro_name));
		// cout << "Macro: " << macro_name << endl;
		return n;

	} else if (t1.type == TT0_MACRO_ARGS) {	// define function like macro
		// cout << "MacroFunc: " << macro_name << endl;
		CMacro* func_macro = new CMacro(MT_FUNC, macro_name);
		n = parse_args(func_macro->args, lexer, n);
		BOOST_ASSERT(!token0s[n].is_eol);

		n = set_macro_body(func_macro->body, lexer, n+1);
		cpp.macros.push_back(func_macro);

		return n;

	} else {
		// cout << "Macro: " << macro_name << endl;
		CMacro* macro = new CMacro(MT_ID, macro_name);
		n = set_macro_body(macro->body, lexer, n);
		cpp.macros.push_back(macro);

		return n;
	}
}

void delete_macro(vector<CMacro*> &macros, string &macroname)
{
	auto macro_i = find_if(macros.begin(), macros.end(), [&](CMacro* m) { return m->name == macroname; } );
	if (macro_i != macros.end()) {
		delete *macro_i;
		macros.erase(macro_i);
	}
}

int parse_args(vector<string>& args, CLexer& lexer, int n)
{
	vector<CToken0> &token0s = lexer.tokens;
	is_not_eol(token0s[n]);	// NG: #define hoge(

	for (;;) {
		n = next_pos(token0s, n);
		CToken0 &t_id = token0s[n];

		if (t_id.type == TT0_ID) {
			is_not_eol(t_id);
			args.push_back(get_str(lexer, &t_id));
			n = next_pos(token0s, n);

			CToken0 &t_next = token0s[n];
			is_not_eol(t_next);

			if (t_next.type == TT0_PUNCTUATOR) {
				int c = get_ch(lexer, &t_next);
				if (c == ',') { // next arg
					continue;

				} else if (c == ')') { // end arg
					break;

				} else {
					BOOST_ASSERT(false);	// error
				}
			}
		}
	}

	return n;
}

int set_macro_body(vector<CToken*> &body, CLexer& lexer, int n)
{
	CToken0 *t = &lexer.tokens[n];
	if (t->is_eol)
		return n;

	do {
		n++;
		t = &lexer.tokens[n];
	} while (!t->is_eol);

	return n;
}

int process_undef(CPreprocessor& cpp, CLexer& lexer, int n)
{
	vector<CToken0> &token0s = lexer.tokens;

	is_not_eol(token0s[n]);

	n = next_pos(token0s, n);
	CToken0 &t = token0s[n];

	if (t.type != TT0_ID) {
		BOOST_ASSERT(false);
	}

	string macro_name = get_str(lexer, &t);
	delete_macro(cpp.macros, macro_name);

	while (!token0s[n].is_eol) {
		n = next_pos(token0s, n);
		if (token0s[n].type != TT0_COMMENT) {
			BOOST_ASSERT(false); 	// output warning
		}
	}

	return n;
}

static int skip_to_nextif(vector<IfInfo>& ifstack, CLexer& lexer, int n);

int process_ifdef(vector<IfInfo>& ifstack, CPreprocessor& cpp, CLexer& lexer, int n, bool is_ndef)
{
	vector<CToken0> &token0s = lexer.tokens;

	is_not_eol(token0s[n]);

	ifstack.emplace_back(&token0s[n]);
	n = next_pos(token0s, n);
	
	CToken0 &t = token0s[n];
	if (t.type != TT0_ID) {
		BOOST_ASSERT(false);
	}

	string macro_name = get_str(lexer, &t);
	auto& macros = cpp.macros;
	auto macro_i = find_if(macros.begin(), macros.end(), [&](CMacro* m) { return m->name == macro_name; } );
	if (is_ndef) {
		ifstack.back().is_fulfilled = macro_i == macros.end();
	} else {
		ifstack.back().is_fulfilled = macro_i != macros.end();
	}

	while (!token0s[n].is_eol) {
		n = next_pos(token0s, n);
		if (token0s[n].type != TT0_COMMENT) {
			BOOST_ASSERT(false); 	// output warning
		}
	}

	if (!ifstack.back().is_fulfilled) {
		n = skip_to_nextif(ifstack, lexer, n);
	}

	return n;
}

int process_else(vector<IfInfo> &ifstack, CLexer& lexer, int n)
{
	if (!ifstack.size()) {
		BOOST_ASSERT(false);
	}

	vector<CToken0> &token0s = lexer.tokens;
	while (!token0s[n].is_eol) {
		n = next_pos(token0s, n);
		if (token0s[n].type != TT0_COMMENT) {
			BOOST_ASSERT(false); 	// output warning
		}
	}

	if (ifstack.back().is_fulfilled) {
		ifstack.back().is_else = true;
		n = skip_to_nextif(ifstack, lexer, n);
	} else {
		ifstack.back().is_fulfilled = true;
		ifstack.back().is_else = true;
	}

	return n;
}

int skip_to_nextif(vector<IfInfo>& ifstack, CLexer& lexer, int n)
{
	vector<CToken0> &token0s = lexer.tokens;
	n++;

	while (n < token0s.size()) {
		CToken0 *t = &token0s[n];
		while (t->type != TT0_MACRO) {
			n++;
			if (n >= token0s.size())
				return n;
			t = &token0s[n];
		}

		BOOST_ASSERT(t->type == TT0_MACRO);

		if (t->is_eol) {
			n++;
			continue;
		}

		int m = next_pos(token0s, n);

		t = &token0s[m];
		if (t->type != TT0_ID) {
			n = m;
			continue;
		}

		string directive = get_str(lexer, t);
		if (directive == "if"
				|| directive == "ifdef"
				|| directive == "ifndef") {
			ifstack.emplace_back(t);
			ifstack.back().is_fulfilled = true;
			n = skip_to_nextif(ifstack, lexer, n);

		} else if (directive == "elif"
				|| directive == "else") {
			if (ifstack.back().is_else) {
				BOOST_ASSERT(false);
			}
			if (ifstack.back().is_fulfilled) {
				n = m+1;
				continue;	// skip
			}
			return n-1;	// back to before '#'

		} else if (directive == "endif") {
			BOOST_ASSERT(ifstack.size());
			ifstack.pop_back();

			while (!token0s[m].is_eol) {
				m = next_pos(token0s, m);
				if (token0s[m].type != TT0_COMMENT) {
					BOOST_ASSERT(false); 	// output warning
				}
			}

			return m;
		}
		n++;
	}

	return n;
}

int next_pos(const vector<CToken0>& token0s, int n)
{
	BOOST_ASSERT(!token0s[n].is_eol);

	// Search next token (Skip comment and end search at eol.)
	const CToken0 *t = NULL;
	do {
		n++;
		t = &token0s[n];
	} while(t->type == TT0_COMMENT && !t->is_eol);

	return n;
}

string get_str(CLexer &lexer, CToken0* t)
{
	BOOST_ASSERT(t->type == TT0_ID || t->type == TT0_STR);
	return lexer.infile.lines[t->line_no-1].substr(t->pos, t->len);
}

int get_ch(CLexer &lexer, CToken0* t)
{
	BOOST_ASSERT(t->type == TT0_PUNCTUATOR);
	BOOST_ASSERT(t->len <= 3);
	int n = t->line_no-1;
	int pos = t->pos;

	if (t->len == 1) {
		return lexer.infile.lines[n][pos];

	} else if (t->len == 2) {
		char c0 = lexer.infile.lines[n][pos];
		char c1 = lexer.infile.lines[n][pos+1];
		return (c0 << 8) | c1;

	} else {
		char c0 = lexer.infile.lines[n][pos];
		char c1 = lexer.infile.lines[n][pos+1];
		char c2 = lexer.infile.lines[n][pos+2];
		return (c0 << 16) | (c1 << 8) | c2;
	}
}

