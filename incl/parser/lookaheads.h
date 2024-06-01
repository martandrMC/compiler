#include "lexer/lexer.h"

#define PEEK lexer_peek()->type
#define CONSUME lexer_next()

#define STMT_FIRSTS \
	/*case*/ TOK_KW_DO: \
	case TOK_KW_IF: \
	case TOK_KW_WHILE: \
	case TOK_KW_RETURN//:

#define EXPR_FIRSTS \
	/*case*/ TOK_OP_PLUS: \
	case TOK_OP_MINUS: \
	case TOK_KW_NOT: \
	case TOK_OPEN_ROUND: \
	case TOK_IDENT: \
	case TOK_LIT_NUM: \
	case TOK_KW_TRUE: \
	case TOK_KW_FALSE: \
	case TOK_KW_NIL//:
