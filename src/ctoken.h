typedef enum {
	TT_ID,
	TT_PUNCTUATOR,
	TT_STR,
	TT_NUMBER,
	TT_COMMENT,
} CTokenType;

class CToken {
public:
	CTokenType type;
	int32_t line_no;
	int16_t pos;
	int16_t len;

	CToken(CTokenType type, int line_no, int pos, int len);
};

class CFileInfo {
public:
	string fname;
	vector<string> lines;

	CFileInfo(string fname);
	string& getLine(int n);
};
