typedef enum {
	TT_ID,
	TT_PUNCTUATOR,
	TT_KEYWORD,
	TT_STR,
	TT_NUMBER,
} CTokenType;

class CToken {
public:
	CTokenType type;
	int32_t lineno;
	int16_t start;
	int16_t end;
	string info;
};

class CFileInfo {
public:
	string fname;
	vector<string> lines;

	CFileInfo(string fname);
	string& getLine(int n);
};
