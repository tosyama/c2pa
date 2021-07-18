typedef enum {
	MT_ID,
	MT_FUNC,
} CMacroType;

class CMacro {
public:
	CMacroType type;
	string name;
	vector<string> args;
	vector<CToken*> body;

	CMacro(CMacroType type, string name);
};

class CPreprocessor {
	vector<CToken*> top_tokens;

public:
	vector<CLexer*> lexers;
	vector<CMacro*> &macros;
	vector<string> &include_paths;

	CPreprocessor(vector<CMacro*> &macros, vector<string> &include_paths)
		: macros(macros), include_paths(include_paths) {}
	~CPreprocessor() {
		for (CLexer* l: lexers) delete l;
	}

	// true: success, false: failed
	bool preprocess(const string& filepath, vector<CToken*> *tokens=NULL);
};

