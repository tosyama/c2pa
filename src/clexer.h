class CLexer {
public:
	CFileInfo &infile;
	vector<CToken> tokens;

	CLexer(CFileInfo &infile);
	vector<CToken>& getTokens();
};
