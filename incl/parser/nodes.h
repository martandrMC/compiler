#define FOREACH_NODE(FN) \
	FN(TYPE) FN(IDENT) FN(LITERAL) \
	FN(OP_UNARY) FN(OP_BINARY) \
	FN(RETURN) FN(WHILE) FN(IF_CASE) \
	FN(INTERNAL) FN(BLOCK) FN(VAR) FN(IF_LIST) FN(CALL)

#define GENERATE_AST_ENUM(VAL) AST_##VAL,
#define GENERATE_AST_STRS(VAL) #VAL,
