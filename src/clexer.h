class CLexer {
public:
	int no;
	CFileInfo infile;
	vector<CToken0> tokens;

	CLexer(CFileInfo &infile);
	vector<CToken0>& scan();
};
