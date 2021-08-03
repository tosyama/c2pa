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
static int process_if(vector<IfInfo> &ifstack, CPreprocessor& cpp, CLexer& lexer, int n);
static int process_ifdef(vector<IfInfo> &ifstack, CPreprocessor& cpp, CLexer& lexer, int n, bool is_ndef = false);
static int process_elif(vector<IfInfo> &ifstack, CLexer& lexer, int n);
static int process_else(vector<IfInfo> &ifstack, CLexer& lexer, int n);
static int process_endif(vector<IfInfo> &ifstack, CLexer& lexer, int n);

static int next_pos(const vector<CToken0>& token0s, int n);

//== CMacro ==
CMacro::CMacro(CMacroType type, string name, int lexer_no)
	: type(type), name(name), lexer_no(lexer_no)
{
}

//== CPreprocessor ==
bool CPreprocessor::loadPredefined(const string& filepath)
{
	CFileInfo finfo(filepath);
	CLexer lexer(finfo);

	vector<CToken0>& token0s = lexer.scan();
	for (int n=0; n<token0s.size(); n++) {
		CToken0 *t = &token0s[n];
		if (t->type == TT0_MACRO) {		// '#'
			n = next_pos(token0s, n);
			BOOST_ASSERT(t != &token0s[n]);
			t = &token0s[n];
			if (t->type == TT0_ID && lexer.get_str(t) == "define") {
				n = process_define(*this, lexer, n);
			} else {
				BOOST_ASSERT(false);
			}
		}
	}

	return true;
}

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
		BOOST_ASSERT(!macro_stack.size());
		CToken0 *t = &token0s[n];
		if (t->type == TT0_MACRO) {		// '#'
			if (t->is_eol) continue;	// null directive

			n = next_pos(token0s, n);
			BOOST_ASSERT(t != &token0s[n]);
			t = &token0s[n];

			if (t->type == TT0_ID) {
				string directive = lexer.get_str(t);
				if (directive == "include") {
					n = process_include(*this, lexer, n, tokens); 
					
				} else if (directive == "include_next") {

				} else if (directive == "define") {
					n = process_define(*this, lexer, n);

				} else if (directive == "undef") {
					n = process_undef(*this, lexer, n);

				} else if (directive == "if") {
					n = process_if(ifstack, *this, lexer, n);

				} else if (directive == "ifdef") {
					n = process_ifdef(ifstack, *this, lexer, n);

				} else if (directive == "ifndef") {
					n = process_ifdef(ifstack, *this, lexer, n, true);

				} else if (directive == "elif") {
					n = process_elif(ifstack, lexer, n);

				} else if (directive == "else") {
					n = process_else(ifstack, lexer, n);

				} else if (directive == "endif") {
					n = process_endif(ifstack, lexer, n);

				} else if (directive == "line") {
				} else if (directive == "pragma") {
				} else if (directive == "error") {
					cout << "Error at " << lexer.infile.fname
						<< ":" << t->line_no << endl;
					BOOST_ASSERT(false);	// error
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

	string inc_path = lexer.get_str(&fname_t);
	string foundpath = CFileInfo::getFilePath(inc_path, lexer.infile.fname, cpp.include_paths);

	if (foundpath == "") {
		BOOST_ASSERT(false);
	}

	tokens->push_back(new CToken(TT_INCLUDE, lexer.no, start));
	cpp.preprocess(foundpath, tokens->back()->info.tokens);
	
	if (!fname_t.is_eol) {
		BOOST_ASSERT(false);
	}

	return n;
}

static void delete_macro(vector<CMacro*> &macros, string &macro_name);
static int parse_params(vector<string>& params, CLexer& lexer, int n);
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
	string macro_name = lexer.get_str(&t);
	delete_macro(cpp.macros, macro_name);

	if (t.is_eol) {	// define only name
		cpp.macros.push_back(new CMacro(MT_OBJ, lexer.get_str(&t), lexer.no));
		return n;
	}

	n = next_pos(token0s, n);
	CToken0 &t1 = token0s[n];

	if (t1.type == TT0_COMMENT) {	// define only name
		BOOST_ASSERT(t1.is_eol);
		cpp.macros.push_back(new CMacro(MT_OBJ, macro_name, lexer.no));
		return n;

	} else if (t1.type == TT0_MACRO_ARGS) {	// define function like macro
		CMacro* func_macro = new CMacro(MT_FUNC, macro_name, lexer.no);
		n = parse_params(func_macro->params, lexer, n);
		n = set_macro_body(func_macro->body, lexer, n);
		cpp.macros.push_back(func_macro);

		return n;

	} else {
		CMacro* macro = new CMacro(MT_OBJ, macro_name, lexer.no);
		CToken* t = lexer.createToken(n);
		macro->body.push_back(t);
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

int parse_params(vector<string>& params, CLexer& lexer, int n)
{
	vector<CToken0> &token0s = lexer.tokens;
	is_not_eol(token0s[n]);	// NG: #define hoge(

	for (;;) {
		n = next_pos(token0s, n);
		CToken0 &t_id = token0s[n];

		if (t_id.type == TT0_ID) {
			is_not_eol(t_id);
			params.push_back(lexer.get_str(&t_id));
			n = next_pos(token0s, n);

			CToken0 &t_next = token0s[n];
			if (t_next.type == TT0_PUNCTUATOR) {
				int c = lexer.get_ch(&t_next);
				if (c == ',') { // next arg
					continue;

				} else if (c == ')') { // end arg
					break;

				} else {
					BOOST_ASSERT(false);	// error
				}
			} else {
				BOOST_ASSERT(false);	// error
			}
		}
	}

	return n;
}

int set_macro_body(vector<CToken*> &body, CLexer& lexer, int n)
{
	CToken0 *t0 = &lexer.tokens[n];
	if (t0->is_eol)
		return n;

	do {
		n++;
		CToken* t = lexer.createToken(n);
		body.push_back(t);
		t0 = &lexer.tokens[n];
	} while (!t0->is_eol);

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

	string macro_name = lexer.get_str(&t);
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
static CMacro* macro_exists(CPreprocessor& cpp, const string& id);
static vector<CToken*> expand_macro_obj(CPreprocessor& cpp, CMacro *m);
static vector<vector<CToken*>> extruct_macro_func_args(CLexer &lexer, int &n);
static vector<vector<CToken*>> extruct_macro_func_args2(vector<CToken*> &tokens, int &n);
static vector<CToken*> expand_macro_func(CPreprocessor& cpp, CMacro *mi, vector<vector<CToken*>> args);

int process_if(vector<IfInfo> &ifstack, CPreprocessor& cpp, CLexer& lexer, int n)
{
	vector<CToken0> &token0s = lexer.tokens;
	is_not_eol(token0s[n]);

	ifstack.emplace_back(&token0s[n]);
	vector<CToken*> condition_tokens;

	while (!token0s[n].is_eol) {	// Read condition expression.
		n = next_pos(token0s, n);
		CToken* t = lexer.createIfMacroToken(n);

		if (t->type == TT_ID) {
			CMacro* m = macro_exists(cpp, *t->info.id);
			if (m) {
				if (m->type == MT_OBJ) {
					vector<CToken*> expanded_tokens = expand_macro_obj(cpp, m);
					condition_tokens.insert(condition_tokens.end(), expanded_tokens.begin(), expanded_tokens.end());

				} else {
					BOOST_ASSERT(m->type == MT_FUNC);
					vector<vector<CToken*>> args = extruct_macro_func_args(lexer, n);
					vector<CToken*> expanded_tokens = expand_macro_func(cpp, m, args);
					condition_tokens.insert(condition_tokens.end(), expanded_tokens.begin(), expanded_tokens.end());
				}
				continue;
			}

		} else if(t->type == TT_KEYWORD && t->info.keyword == TK_DEFINED) {
			// process of "defined" expression
			int nn = n;
			delete t;
			is_not_eol(token0s[n]);
			string macro_name;
			n = next_pos(token0s, n);
			CToken0& t0 = token0s[n];
			if (t0.type == TT0_ID) {
				// defined ID
				macro_name = lexer.get_str(&t0);

			} else if (t0.type == TT0_PUNCTUATOR && lexer.get_ch(&t0) == '(' && !t0.is_eol) {
				// defined (ID)
				n = next_pos(token0s, n);
				t0 = token0s[n];
				is_not_eol(token0s[n]);
				if (t0.type == TT0_ID) {
					macro_name = lexer.get_str(&t0);
				} else {
					BOOST_ASSERT(false);
				}
				n = next_pos(token0s, n);
				t0 = token0s[n];
				if (t0.type != TT0_PUNCTUATOR || lexer.get_ch(&t0) != ')') {
					BOOST_ASSERT(false);
				}
				
			} else {
				BOOST_ASSERT(false);
			}

			t = new CToken(TT_INT, lexer.no, nn);
			CMacro* m = macro_exists(cpp, macro_name);
			if (m) {
				t->info.intval = 1;
			} else {
				t->info.intval = 0;
			}

		}

		condition_tokens.push_back(t);
	}

	cout << "if:";
	for (CToken* t: condition_tokens) {
		if (t->type == TT_ID)
			cout << " " << *t->info.id;
		else if (t->type == TT_PUNCTUATOR)
			cout << " *";
		else if (t->type == TT_INT || t->type == TT_UINT || t->type == TT_LONG || t->type == TT_ULONG)
			cout << " 0";
		else if (t->type == TT_KEYWORD)
			cout << " d";
		else 
			cout << " ?";
	}
	cout << endl;
	
	// ifstack.back().is_fulfilled = ?

	if (!ifstack.back().is_fulfilled) {
		n = skip_to_nextif(ifstack, lexer, n);
	}

	return n;
}

vector<CToken*> expand_macro_obj(CPreprocessor& cpp, CMacro *m)
{

	auto mi = find(cpp.macro_stack.begin(), cpp.macro_stack.end(), m);
	if (mi != cpp.macro_stack.end()) {
		BOOST_ASSERT(false); 	// recursive macro
	}

	vector<CToken*> tokens;

	cpp.macro_stack.push_back(m);

	for (int n=0; n<m->body.size(); n++) {
		CToken *t = m->body[n];
		if (t->type == TT_ID) {
			CMacro* m = macro_exists(cpp, *t->info.id);
			if (m) {
				if (m->type == MT_OBJ) {
					vector<CToken*> expanded_tokens = expand_macro_obj(cpp, m);
					tokens.insert(tokens.end(), expanded_tokens.begin(), expanded_tokens.end());
				} else {
					BOOST_ASSERT(m->type == MT_FUNC);
					vector<vector<CToken*>> args = extruct_macro_func_args2(m->body, n);
					vector<CToken*> expanded_tokens = expand_macro_func(cpp, *mi, args);
					tokens.insert(tokens.end(), expanded_tokens.begin(), expanded_tokens.end());
				}
				continue;
			}
		}
		CToken *tt = new CToken(*t);
		tokens.push_back(tt);

	}

	cpp.macro_stack.pop_back();

	return tokens;
}

vector<CToken*> expand_macro_func(CPreprocessor& cpp, CMacro *m, vector<vector<CToken*>> args)
{
	auto mi = find(cpp.macro_stack.begin(), cpp.macro_stack.end(), m);
	if (mi != cpp.macro_stack.end()) {
		BOOST_ASSERT(false); 	// recursive macro
	}

	if (m->params.size() != args.size()) {
		BOOST_ASSERT(false);
	}

	vector<CToken*> tokens;
	cpp.macro_stack.push_back(m);

	for (int n=0; n<m->body.size(); n++) {
		CToken *t = m->body[n];
		if (t->type == TT_ID) {
			int p;
			for (p=0; p < m->params.size(); p++) {
				if (*t->info.id == m->params[p]) {
					for (CToken *at: args[p]) {
						if (at->type == TT_ID) {
							CMacro* m = macro_exists(cpp, *at->info.id);
							if (m) {
								if (m->type == MT_OBJ) {
									vector<CToken*> expanded_tokens = expand_macro_obj(cpp, m);
									tokens.insert(tokens.end(), expanded_tokens.begin(), expanded_tokens.end());
								} else {
									BOOST_ASSERT(m->type == MT_FUNC);
									vector<vector<CToken*>> args = extruct_macro_func_args2(m->body, n);
									vector<CToken*> expanded_tokens = expand_macro_func(cpp, *mi, args);
									tokens.insert(tokens.end(), expanded_tokens.begin(), expanded_tokens.end());
								}
								continue;
							}
						}

						CToken* att = new CToken(*at);
						tokens.push_back(att);
					}
					break;
				}
			}

			if (p < m->params.size())
				continue;

			CMacro* m = macro_exists(cpp, *t->info.id);
			if (m) {
				if (m->type == MT_OBJ) {
					vector<CToken*> expanded_tokens = expand_macro_obj(cpp, m);
					tokens.insert(tokens.end(), expanded_tokens.begin(), expanded_tokens.end());
				} else {
					BOOST_ASSERT(m->type == MT_FUNC);
					vector<vector<CToken*>> args = extruct_macro_func_args2(m->body, n);
					vector<CToken*> expanded_tokens = expand_macro_func(cpp, *mi, args);
					tokens.insert(tokens.end(), expanded_tokens.begin(), expanded_tokens.end());
				}
				continue;
			}
		}
		CToken *tt = new CToken(*t);
		tokens.push_back(tt);

	}

	cpp.macro_stack.pop_back();

	return tokens;
}

vector<vector<CToken*>> extruct_macro_func_args(CLexer &lexer, int &n)
{
	vector<vector<CToken*>> args;

	vector<CToken0> &token0s = lexer.tokens;
	is_not_eol(token0s[n]);

	n++;
	CToken* t = lexer.createIfMacroToken(n);
	if (t->type != TT_PUNCTUATOR || t->info.punc != '(') {
		BOOST_ASSERT(false);
	}
	
	int blace_level = 1;
	vector<CToken*> arg;

	do {
		n = next_pos(token0s, n);
		CToken* t = lexer.createIfMacroToken(n);
		if (t->type == TT_PUNCTUATOR) {
			if (t->info.punc == '(') {
				blace_level++;
			} else if (t->info.punc == ')') {
				blace_level--;
				if (blace_level == 0) {
					delete t;
					break;
				}
			} else if (t->info.punc == ',' && blace_level == 1) {
				delete t;
				args.push_back(arg);
				continue;
			}
		}
		arg.push_back(t);

	} while (!token0s[n].is_eol);
	args.push_back(arg);

	return args;
}

vector<vector<CToken*>> extruct_macro_func_args2(vector<CToken*> &tokens, int &n)
{
	vector<vector<CToken*>> args;
	BOOST_ASSERT(tokens[n]->type == TT_PUNCTUATOR);
	BOOST_ASSERT(tokens[n]->info.punc == '(');

	int blace_level = 1;
	vector<CToken*> arg;

	n++;
	for (; n < tokens.size(); n++) {
		CToken *t = tokens[n];
		if (t->type == TT_PUNCTUATOR) {
			if (t->info.punc == '(') {
				blace_level++;
			} else if (t->info.punc == ')') {
				blace_level--;
				if (blace_level == 0) {
					break;
				}
			} else if (t->info.punc == ',' && blace_level == 1) {
				args.push_back(arg);
				continue;
			}
		}
		arg.push_back(new CToken(*t));
	}
	args.push_back(arg);

	return args;
}

CMacro* macro_exists(CPreprocessor& cpp, const string& id)
{
	auto mi = find_if(cpp.macros.begin(), cpp.macros.end(),
			[id](CMacro* m) { return m->name == id; });

	if (mi != cpp.macros.end()) {
		return *mi;
	}
	return NULL;
}

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

	string macro_name = lexer.get_str(&t);
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

int process_elif(vector<IfInfo> &ifstack, CLexer& lexer, int n)
{
	if (!ifstack.size()) {
		BOOST_ASSERT(false);
	}

	BOOST_ASSERT(!ifstack.back().is_fulfilled);
	BOOST_ASSERT(!ifstack.back().is_else);

	vector<CToken0> &token0s = lexer.tokens;
	is_not_eol(token0s[n]);

	// ifstack.back().is_fulfilled = ?
	
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

int process_endif(vector<IfInfo> &ifstack, CLexer& lexer, int n)
{
	vector<CToken0> &token0s = lexer.tokens;

	if (!ifstack.size()) {
		BOOST_ASSERT(false);
	}

	ifstack.pop_back();

	while (!token0s[n].is_eol) {
		n = next_pos(token0s, n);
		if (token0s[n].type != TT0_COMMENT) {
			BOOST_ASSERT(false); 	// output warning
		}
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

		string directive = lexer.get_str(t);
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
			return process_endif(ifstack, lexer, m);
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

