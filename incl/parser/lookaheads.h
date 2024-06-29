#include "lexer/lexer.h"

#define STMT_FIRSTS \
	/*case*/ TOK_KW_DO: \
	case TOK_KW_IF: \
	case TOK_KW_WHILE: \
	case TOK_KW_RETURN//:

#define EXPR_FIRSTS \
	/*case*/ TOK_OP_PLUS: \
	case TOK_OP_MINUS: \
	case TOK_KW_NOT: \
	case TERM_FIRSTS//:

#define TERM_FIRSTS \
	/*case*/ TOK_OPEN_ROUND: \
	case TOK_IDENT: \
	case TOK_LIT_NUM: \
	case TOK_KW_TRUE: \
	case TOK_KW_FALSE: \
	case TOK_KW_NIL//:

#define EXPR_FOLLOWS \
	/*case*/ TOK_COMMA: \
	case TOK_COLON: \
	case TOK_SEMICOLON: \
	case TOK_CLOSE_ROUND: \
	case TOK_KW_END: \
	case TOK_KW_ELIF: \
	case TOK_KW_ELSE//:

#define TERM_FOLLOWS \
	/*case*/ TOK_OP_MULT: \
	case TOK_OP_DIV: \
	case TOK_OP_MOD: \
	case TOK_OP_PLUS: \
	case TOK_OP_MINUS: \
	case TOK_OP_COMPARE: \
	case TOK_KW_AND: \
	case TOK_KW_OR: \
	case TOK_OP_ASSIGN: \
	case TOK_OP_ASSIGN_ALT: \
	case EXPR_FOLLOWS//:
