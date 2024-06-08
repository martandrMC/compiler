#include "lexer/lexer.h"

#define PEEK lexer_peek()->type
#define CONSUME lexer_next()

#define TERM_FIRSTS \
	/*case*/ TOK_OPEN_ROUND: \
	case TOK_IDENT: \
	case TOK_LIT_NUM: \
	case TOK_KW_TRUE: \
	case TOK_KW_FALSE: \
	case TOK_KW_NIL//:

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

#define PREC_0_FOLLOWS \
	/*case*/ TOK_COMMA: \
	case TOK_COLON: \
	case TOK_SEMICOLON: \
	case TOK_CLOSE_ROUND: \
	case TOK_KW_END: \
	case TOK_KW_ELSE//:

#define PREC_1_FOLLOWS \
	/*case*/ TOK_OP_ASSIGN: \
	case TOK_OP_ASSIGN_ALT: \
	case PREC_0_FOLLOWS//:

#define PREC_2_FOLLOWS \
	/*case*/ TOK_KW_AND: \
	case TOK_KW_OR: \
	case PREC_1_FOLLOWS//:

#define PREC_3_FOLLOWS \
	/*case*/ TOK_OP_COMPARE: \
	case PREC_2_FOLLOWS//:

#define PREC_4_FOLLOWS \
	/*case*/ TOK_OP_PLUS: \
	case TOK_OP_MINUS: \
	case PREC_3_FOLLOWS//:

#define PREC_5_FOLLOWS \
	/*case*/ TOK_OP_MULT: \
	case TOK_OP_DIV: \
	case TOK_OP_MOD: \
	case PREC_4_FOLLOWS//:
