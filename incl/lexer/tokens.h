#define FOREACH_TOKEN(FN) \
	/* Internal */ \
	FN(ERROR) FN(EOF) \
	/* Structural */ \
	FN(OPEN_ROUND) FN(CLOSE_ROUND) \
	FN(COMMA) FN(COLON) FN(SEMICOLON) \
	/* Operators */ \
	FN(OP_ASSIGN) FN(OP_ASSIGN_ALT) FN(OP_COMPARE) \
	FN(OP_PLUS) FN(OP_MINUS) FN(OP_MULT) FN(OP_DIV) FN(OP_MOD) \
	/* Keywords */ \
	FN(KW_DO) FN(KW_END) FN(KW_VAR) FN(KW_RETURN) \
	FN(KW_IF) FN(KW_ELIF) FN(KW_ELSE) FN(KW_WHILE) \
	FN(KW_AND) FN(KW_OR) FN(KW_NOT) \
	FN(KW_TRUE) FN(KW_FALSE) FN(KW_NIL) \
	/* Base Types */ \
	FN(TYPE_NAT) FN(TYPE_INT) FN(TYPE_BOOL) \
	/* Others */ \
	FN(IDENT) FN(LIT_NUM)

#define GENERATE_TOK_ENUM(VAL) TOK_##VAL,
#define GENERATE_TOK_STRS(VAL) #VAL,
