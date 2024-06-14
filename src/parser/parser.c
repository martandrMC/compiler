#include "ast.h"
#include "lookaheads.h"
#include "parser.h"

#include "common/io.h"
#include "lexer/lexer.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef enum se_variant {
	STMT_EXPR,
	STMT_EXPR_SEM,
	STMT_EXPR_END
} se_variant_t;

typedef enum log_action {
	ACTION_PRINT,
	ACTION_ENTER,
	ACTION_EXIT
} log_action_t;

static struct parser_state {
	ast_t ast;
} ps;

// Internal Functions (Helpers) //

static void _panic_common(token_t *problem) {
	size_t line = 1;
	char *file_base = lexer_get_src().string;
	char *problem_base = problem->content.string;
	char *last_nl = file_base - 1;

	for(char *c = file_base; c < problem->content.string; c++)
		if(*c == '\n') line++, last_nl = c;

	printf("[%lu:%lu] Errant token encountered: ", line, (uintptr_t) (problem_base - last_nl));
	if(problem->type == TOK_EOF) printf("EOF");
	else printf("\"%.*s\"", (int) problem->content.size, problem_base);
}

static void _panic(token_t *problem) {
	_panic_common(problem);
	putchar('\n');
	exit(EXIT_FAILURE);
}

static void _panic_expect(token_t *problem, const char *message) {
	_panic_common(problem);
	printf(", expected: %s\n", message);
	exit(EXIT_FAILURE);
}

static token_t *_expect(token_type_t type) {
	token_t *next = CONSUME;
	if(next->type != type) {
		_panic_common(next);
		printf(", expected: %s\n", token_type_strs[type]);
		exit(EXIT_FAILURE);
	}
	return next;
}

// Internal Function Decls (Non-Terminals) //

static ast_node_t *_nt_block(void);
static void _nt_var_init(void);
static void _nt_var_expr_next(void);
static void _nt_var_stmt_next(void);
static void _nt_type(void);
static void _nt_stmt_expr(void);
static void _nt_stmt_expr_sem(void);
static void _nt_stmt_expr_end(void);
static void _nt_stmt(void);
static void _nt_if_next(void);

static void _nt_prec_0(void);
static void _nt_prec_0_(void);
static void _nt_prec_1(void);
static void _nt_prec_1_(void);
static void _nt_prec_2(void);
static void _nt_prec_2_(void);
static bool _nt_unaries_2(void);
static void _nt_prec_3(void);
static void _nt_prec_3_(void);
static void _nt_prec_4(void);
static void _nt_prec_4_(void);
static void _nt_prec_5(void);
static bool _nt_unaries_5(void);
static void _nt_term(void);
static void _nt_term_(void);
static void _nt_func(void);
static void _nt_func_(void);

// Internal Functions Defs (Non-Terminals) //

static ast_node_t *_nt_block(void) {
	ast_node_t *block = ast_lnode_new(&ps.ast, 4, AST_BLOCK, EMPTY_STRING);
loop:
	switch(PEEK) {
		case TOK_KW_VAR: CONSUME;
			_nt_type();
			_expect(TOK_IDENT);
			_expect(TOK_OP_ASSIGN);
			_nt_var_init();
			goto loop;
		case STMT_FIRSTS:
		case EXPR_FIRSTS:
			_nt_stmt_expr_sem();
			goto loop;
		case TOK_KW_END:
		case TOK_EOF:
			break;
		default: _panic_expect(CONSUME, "\"var\", statement or expression");
	}
	return block;
}

static void _nt_var_init(void) {
	switch(PEEK) {
		case STMT_FIRSTS:
			_nt_stmt();
			_nt_var_stmt_next();
			break;
		case EXPR_FIRSTS:
			_nt_prec_0();
			_nt_var_expr_next();
			break;
		default: _panic_expect(CONSUME, "statement or expression");
	}
}

static void _nt_var_expr_next(void) {
	switch(PEEK) {
		case TOK_COMMA: CONSUME;
			_expect(TOK_IDENT);
			_expect(TOK_OP_ASSIGN);
			_nt_var_init();
			break;
		case TOK_SEMICOLON: CONSUME;
			break;
		default: _panic_expect(CONSUME, "\",\" or \";\"");
	}
}

static void _nt_var_stmt_next(void) {
	switch(PEEK) {
		case TOK_COMMA: CONSUME;
			_expect(TOK_IDENT);
			_expect(TOK_OP_ASSIGN);
			_nt_var_init();
			break;
		case TOK_KW_END:
		case TOK_EOF:
		case TOK_KW_VAR:
		case STMT_FIRSTS:
		case EXPR_FIRSTS:
			break;
		default: _panic_expect(CONSUME, "\",\" or \"end\" or EOF or block member");
	}
}

static void _nt_type(void) {
	switch(PEEK) {
		case TOK_TYPE_NAT: CONSUME;
			break;
		case TOK_TYPE_INT: CONSUME;
			break;
		case TOK_TYPE_BOOL: CONSUME;
			break;
		default: _panic_expect(CONSUME, "\"nat\" or \"int\" or \"bool\"");
	}
}

static void _nt_stmt_expr(void) {
	switch(PEEK) {
		case STMT_FIRSTS:
			_nt_stmt();
			break;
		case EXPR_FIRSTS:
			_nt_prec_0();
			break;
		default: _panic(CONSUME);
	}
}

static void _nt_stmt_expr_sem(void) {
	switch(PEEK) {
		case STMT_FIRSTS:
			_nt_stmt();
			break;
		case EXPR_FIRSTS:
			_nt_prec_0();
			_expect(TOK_SEMICOLON);
			break;
		default: _panic(CONSUME);
	}
}

static void _nt_stmt_expr_end(void) {
	switch(PEEK) {
		case STMT_FIRSTS:
			_nt_stmt();
			break;
		case EXPR_FIRSTS:
			_nt_prec_0();
			_expect(TOK_KW_END);
			break;
		default: _panic(CONSUME);
	}
}

static void _nt_stmt(void) {
	switch(PEEK) {
		case TOK_KW_DO: CONSUME;
			_nt_block();
			_expect(TOK_KW_END);
			break;
		case TOK_KW_IF: CONSUME;
			_nt_stmt_expr();
			_expect(TOK_COLON);
			_nt_stmt_expr();
			_nt_if_next();
			break;
		case TOK_KW_WHILE: CONSUME;
			_nt_stmt_expr();
			_expect(TOK_COLON);
			_nt_stmt_expr_end();
			break;
		case TOK_KW_RETURN: CONSUME;
			_nt_stmt_expr_sem();
			break;
		default: _panic(CONSUME);
	}
}

static void _nt_if_next(void) {
	switch(PEEK) {
		case TOK_KW_ELSE: CONSUME;
			_nt_stmt_expr_end();
			break;
		case TOK_KW_END: CONSUME;
			break;
		default: _panic(CONSUME);
	}
}

static void _nt_prec_0(void) {
	switch(PEEK) {
		case TOK_KW_NOT:
		case TOK_OP_PLUS:
		case TOK_OP_MINUS:
		case TERM_FIRSTS:
			_nt_prec_1();
			_nt_prec_0_();
			break;
		default: _panic(CONSUME);
	}
}

static void _nt_prec_0_(void) {
loop:
	switch(PEEK) {
		case TOK_OP_ASSIGN:
		case TOK_OP_ASSIGN_ALT: CONSUME;
			_nt_prec_1();
			goto loop;
		case PREC_0_FOLLOWS:
			break;
		default: _panic(CONSUME);
	}
}

static void _nt_prec_1(void) {
	switch(PEEK) {
		case TOK_KW_NOT:
		case TOK_OP_PLUS:
		case TOK_OP_MINUS:
		case TERM_FIRSTS:
			_nt_prec_2();
			_nt_prec_1_();
			break;
		default: _panic(CONSUME);
	}
}

static void _nt_prec_1_(void) {
loop:
	switch(PEEK) {
		case TOK_KW_AND: CONSUME;
			_nt_prec_2();
			goto loop;
		case TOK_KW_OR: CONSUME;
			_nt_prec_2();
			goto loop;
		case PREC_1_FOLLOWS:
			break;
		default: _panic(CONSUME);
	}
}

static void _nt_prec_2(void) {
	switch(PEEK) {
		case TOK_KW_NOT:
		case TOK_OP_PLUS:
		case TOK_OP_MINUS:
		case TERM_FIRSTS: ;
			_nt_unaries_2();
			_nt_prec_3();
			_nt_prec_2_();
			break;
		default: _panic(CONSUME);
	}
}

static void _nt_prec_2_(void) {
loop:
	switch(PEEK) {
		case TOK_OP_COMPARE: CONSUME;
			_nt_prec_3();
			goto loop;
		case PREC_2_FOLLOWS:
			break;
		default: _panic(CONSUME);
	}
}

static bool _nt_unaries_2(void) {
	switch(PEEK) {
		case TOK_KW_NOT: CONSUME;
			return true;
		case TOK_OP_PLUS:
		case TOK_OP_MINUS:
		case TERM_FIRSTS:
			return false;
		default: _panic(CONSUME);
	}
	return false;
}

static void _nt_prec_3(void) {
	switch(PEEK) {
		case TOK_OP_PLUS:
		case TOK_OP_MINUS:
		case TERM_FIRSTS:
			_nt_prec_4();
			_nt_prec_3_();
			break;
		default: _panic(CONSUME);
	}
}

static void _nt_prec_3_(void) {
loop:
	switch(PEEK) {
		case TOK_OP_PLUS: CONSUME;
			_nt_prec_4();
			goto loop;
		case TOK_OP_MINUS: CONSUME;
			_nt_prec_4();
			goto loop;
		case PREC_3_FOLLOWS:
			break;
		default: _panic(CONSUME);
	}
}

static void _nt_prec_4(void) {
	switch(PEEK) {
		case TOK_OP_PLUS:
		case TOK_OP_MINUS:
		case TERM_FIRSTS:
			_nt_prec_5();
			_nt_prec_4_();
			break;
		default: _panic(CONSUME);
	}
}

static void _nt_prec_4_(void) {
loop:
	switch(PEEK) {
		case TOK_OP_MULT: CONSUME;
			_nt_prec_5();
			goto loop;
		case TOK_OP_DIV: CONSUME;
			_nt_prec_5();
			goto loop;
		case TOK_OP_MOD: CONSUME;
			_nt_prec_5();
			goto loop;
		case PREC_4_FOLLOWS:
			break;
		default: _panic(CONSUME);
	}
}

static void _nt_prec_5(void) {
	switch(PEEK) {
		case TOK_OP_PLUS:
		case TOK_OP_MINUS:
		case TERM_FIRSTS: ;
			_nt_unaries_5();
			_nt_term();
			break;
		default: _panic(CONSUME);
	}
}

static bool _nt_unaries_5(void) {
	switch(PEEK) {
		case TOK_OP_PLUS: CONSUME;
			return true;
		case TOK_OP_MINUS: CONSUME;
			return true;
		case TERM_FIRSTS:
			return false;
		default: _panic(CONSUME);
	}
	return false;
}

static void _nt_term(void) {
	switch(PEEK) {
		case TOK_OPEN_ROUND: CONSUME;
			_nt_prec_0();
			_expect(TOK_CLOSE_ROUND);
			break;
		case TOK_IDENT: CONSUME;
			_nt_term_();
			break;
		case TOK_LIT_NUM: CONSUME;
			break;
		case TOK_KW_TRUE: CONSUME;
			break;
		case TOK_KW_FALSE: CONSUME;
			break;
		case TOK_KW_NIL: CONSUME;
			break;
		default: _panic(CONSUME);
	}
}

static void _nt_term_(void) {
	switch(PEEK) {
		case TOK_OPEN_ROUND: CONSUME;
			_nt_func();
			_expect(TOK_CLOSE_ROUND);
			break;
		case PREC_5_FOLLOWS:
			break;
		default: _panic(CONSUME);
	}
}

static void _nt_func(void) {
	switch(PEEK) {
		case STMT_FIRSTS:
		case EXPR_FIRSTS:
			_nt_stmt_expr();
			_nt_func_();
			break;
		case TOK_CLOSE_ROUND:
			break;
		default: _panic(CONSUME);
	}
}

static void _nt_func_(void) {
loop:
	switch(PEEK) {
		case TOK_COMMA: CONSUME;
			_nt_stmt_expr();
			goto loop;
		case TOK_CLOSE_ROUND:
			break;
		default: _panic(CONSUME);
	}
}

// External Functions //

void parser_start(void) {
	ps.ast = ast_tree_new();
	_nt_block();
	_expect(TOK_EOF);
}
