typedef enum {
	TT0_ID,
	TT0_PUNCTUATOR,
	TT0_STR,
	TT0_NUMBER,
	TT0_COMMENT,
	TT0_MACRO,
	TT0_MACRO_ARGS,
} CToken0Type;

class CToken0 {
public:
	CToken0Type type;
	int32_t line_no;
	int16_t pos;
	int16_t len;
	bool is_eol;

	CToken0(CToken0Type type, int line_no, int pos, int len);
};

typedef enum {
	TT_ID,
	TT_PUNCTUATOR,
	TT_INCLUDE,
} CTokenType;

class CToken {
public:
	CTokenType type;
	int16_t lexer_no;
	int start_token_no;
	int end_token_no;
	union {
		vector<CToken*> *tokens;	// for TT_INCLUDE
	} info;

	CToken(CTokenType type, int lexer_no, int start_token_no, int end_token_no);
	~CToken();
};
