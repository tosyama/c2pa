typedef enum {
	MT_OBJ,
	MT_FUNC,
} CMacroType;

class CMacro {
public:
	CMacroType type;
	string name;
	vector<string> params;
	vector<CToken*> body;
	int lexer_no;

	CMacro(CMacroType type, string name, int lexer_no);
};

class CPreprocessor {
	vector<CToken*> top_tokens;

public:
	vector<CLexer*> lexers;
	vector<CMacro*> &macros;
	vector<CMacro*> macro_stack;
	vector<string> &include_paths;

	CPreprocessor(vector<CMacro*> &macros, vector<string> &include_paths)
		: macros(macros), include_paths(include_paths) {}
	~CPreprocessor() {
		for (CLexer* l: lexers) delete l;
	}

	// true: success, false: failed
	bool loadPredefined(const string& filepath);
	bool preprocess(const string& filepath, vector<CToken*> *tokens=NULL);
};

