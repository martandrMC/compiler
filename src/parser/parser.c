#include "parser.h"
#include "lookaheads.h"
#include "ast.h"

#include "lexer/lexer.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

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

// Internal Functions (Helpers) //

static void _log_tree(log_action_t action, ...) {
	static unsigned nesting = 0;
	va_list args;
	switch(action) {
		case ACTION_ENTER:
			nesting++;
			break;
		case ACTION_EXIT:
			if(nesting > 0)
			nesting--;
			break;
		case ACTION_PRINT:
			va_start(args, action);
			const char *fmt = va_arg(args, char *);
			for(unsigned i=0; i<nesting; i++) 
				printf("│ ");
			printf("├─");
			vprintf(fmt, args);
			va_end(args);
			break;
	}
}

static void _panic(token_t *problem) {
	printf("\nErrant token encountered: \"%.*s\"\n",
		(int) problem->content.size,
		problem->content.string
	);
	exit(EXIT_FAILURE);
}

static token_t *_expect(token_type_t type) {
	token_t *next = CONSUME;
	if(next->type != type) _panic(next);
	return next;
}

// Internal Function Decls (Non-Terminals) //

static void _nt_block(void);
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

static void _nt_block(void) {
	printf("BLOCK\n");
	_log_tree(ACTION_ENTER);
loop:
	switch(PEEK) {
		case TOK_KW_VAR: CONSUME;
			_log_tree(ACTION_PRINT, "VAR ");
			_nt_type();
			_log_tree(ACTION_ENTER);
			token_t *id = _expect(TOK_IDENT);
			_log_tree(ACTION_PRINT, "%.*s",
				(int) id->content.size,
				id->content.string
			);
			_expect(TOK_OP_ASSIGN);
			_nt_var_init();
			_log_tree(ACTION_EXIT);
			goto loop;
		case STMT_FIRSTS:
		case EXPR_FIRSTS:
			_nt_stmt_expr_sem();
			goto loop;
		case TOK_KW_END:
		case TOK_EOF:
			_log_tree(ACTION_EXIT);
			break;
		default: _panic(CONSUME);
	}
}

static void _nt_var_init(void) {
	printf(" ASSIGN ");
	switch(PEEK) {
		case STMT_FIRSTS:
			_nt_stmt();
			_nt_var_stmt_next();
			break;
		case EXPR_FIRSTS:
			_nt_prec_0();
			printf("\n");
			_nt_var_expr_next();
			break;
		default: _panic(CONSUME);
	}
}

static void _nt_var_expr_next(void) {
	switch(PEEK) {
		case TOK_COMMA: CONSUME;
			token_t *id = _expect(TOK_IDENT);
			_log_tree(ACTION_PRINT, "%.*s",
				(int) id->content.size,
				id->content.string
			);
			_expect(TOK_OP_ASSIGN);
			_nt_var_init();
			break;
		case TOK_SEMICOLON: CONSUME;
			break;
		default: _panic(CONSUME);
	}
}

static void _nt_var_stmt_next(void) {
	switch(PEEK) {
		case TOK_COMMA: CONSUME;
			token_t *id = _expect(TOK_IDENT);
			_log_tree(ACTION_PRINT, "%.*s",
				(int) id->content.size,
				id->content.string
			);
			_expect(TOK_OP_ASSIGN);
			_nt_var_init();
			break;
		case TOK_KW_END:
		case TOK_EOF:
		case STMT_FIRSTS:
		case EXPR_FIRSTS:
			break;
		default: _panic(CONSUME);
	}
}

static void _nt_type(void) {
	switch(PEEK) {
		case TOK_TYPE_NAT: CONSUME;
			printf("nat\n");
			break;
		case TOK_TYPE_INT: CONSUME;
			printf("int\n");
			break;
		case TOK_TYPE_BOOL: CONSUME;
			printf("bool\n");
			break;
		default: _panic(CONSUME);
	}
}

static void _nt_stmt_expr(void) {
	switch(PEEK) {
		case STMT_FIRSTS:
			_log_tree(ACTION_PRINT, "");
			_nt_stmt();
			break;
		case EXPR_FIRSTS:
			_log_tree(ACTION_PRINT, "");
			_nt_prec_0();
			printf("\n");
			break;
		default: _panic(CONSUME);
	}
}

static void _nt_stmt_expr_sem(void) {
	switch(PEEK) {
		case STMT_FIRSTS:
			_log_tree(ACTION_PRINT, "");
			_nt_stmt();
			break;
		case EXPR_FIRSTS:
			_log_tree(ACTION_PRINT, "");
			_nt_prec_0();
			printf("\n");
			_expect(TOK_SEMICOLON);
			break;
		default: _panic(CONSUME);
	}
}

static void _nt_stmt_expr_end(void) {
	switch(PEEK) {
		case STMT_FIRSTS:
			_log_tree(ACTION_PRINT, "");
			_nt_stmt();
			break;
		case EXPR_FIRSTS:
			_log_tree(ACTION_PRINT, "");
			_nt_prec_0();
			printf("\n");
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
			printf("IF\n");
			_log_tree(ACTION_ENTER);
			_nt_stmt_expr();
			_expect(TOK_COLON);
			_nt_stmt_expr();
			_nt_if_next();
			break;
		case TOK_KW_WHILE: CONSUME;
			printf("WHILE\n");
			_log_tree(ACTION_ENTER);
			_nt_stmt_expr();
			_expect(TOK_COLON);
			_nt_stmt_expr_end();
			_log_tree(ACTION_EXIT);
			break;
		case TOK_KW_RETURN: CONSUME;
			printf("RETURN\n");
			_log_tree(ACTION_ENTER);
			_nt_stmt_expr_sem();
			_log_tree(ACTION_EXIT);
			break;
		default: _panic(CONSUME);
	}
}

static void _nt_if_next(void) {
	_log_tree(ACTION_EXIT);
	switch(PEEK) {
		case TOK_KW_ELSE: CONSUME;
			_log_tree(ACTION_PRINT, "ELSE\n");
			_log_tree(ACTION_ENTER);
			_nt_stmt_expr_end();
			break;
		case TOK_KW_END: CONSUME;
			break;
		default: _panic(CONSUME);
	}
}

static void _nt_prec_0(void) {
	printf("[");
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
	printf("]");
}

static void _nt_prec_0_(void) {
loop:
	switch(PEEK) {
		case TOK_OP_ASSIGN:
		case TOK_OP_ASSIGN_ALT: ;
			string_t variant = CONSUME->content;
			printf(" ");
			_nt_prec_1();
			printf(" %.*s",
				(int) variant.size,
				variant.string
			);
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
			printf(" ");
			_nt_prec_2();
			printf(" and");
			goto loop;
		case TOK_KW_OR: CONSUME;
			printf(" ");
			_nt_prec_2();
			printf("or");
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
			bool found = _nt_unaries_2();
			if(found) printf("(");
			_nt_prec_3();
			_nt_prec_2_();
			if(found) printf(")");
			break;
		default: _panic(CONSUME);
	}
}

static void _nt_prec_2_(void) {
loop:
	switch(PEEK) {
		case TOK_OP_COMPARE: ;
			string_t variant = CONSUME->content;
			printf(" ");
			_nt_prec_3();
			printf(" %.*s",
				(int) variant.size,
				variant.string
			);
			goto loop;
		case PREC_2_FOLLOWS:
			break;
		default: _panic(CONSUME);
	}
}

static bool _nt_unaries_2(void) {
	switch(PEEK) {
		case TOK_KW_NOT: CONSUME;
			printf("not");
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
			printf(" ");
			_nt_prec_4();
			printf(" +");
			goto loop;
		case TOK_OP_MINUS: CONSUME;
			printf(" ");
			_nt_prec_4();
			printf(" -");
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
			printf(" ");
			_nt_prec_5();
			printf(" *");
			goto loop;
		case TOK_OP_DIV: CONSUME;
			printf(" ");
			_nt_prec_5();
			printf(" /");
			goto loop;
		case TOK_OP_MOD: CONSUME;
			printf(" ");
			_nt_prec_5();
			printf(" %%");
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
			bool found = _nt_unaries_5();
			if(found) printf("(");
			_nt_term();
			if(found) printf(")");
			break;
		default: _panic(CONSUME);
	}
}

static bool _nt_unaries_5(void) {
	switch(PEEK) {
		case TOK_OP_PLUS: CONSUME;
			printf("+");
			return true;
		case TOK_OP_MINUS: CONSUME;
			printf("-");
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
		case TOK_IDENT: ;
			string_t id = CONSUME->content;
			printf("%.*s",
				(int) id.size,
				id.string
			);
			_nt_term_();
			break;
		case TOK_LIT_NUM: ;
			string_t num = CONSUME->content;
			printf("%.*s",
				(int) num.size,
				num.string
			);
			break;
		case TOK_KW_TRUE: CONSUME;
			printf("`true`");
			break;
		case TOK_KW_FALSE: CONSUME;
			printf("`false`");
			break;
		case TOK_KW_NIL: CONSUME;
			printf("`nil`");
			break;
		default: _panic(CONSUME);
	}
}

static void _nt_term_(void) {
	switch(PEEK) {
		case TOK_OPEN_ROUND: CONSUME;
			_log_tree(ACTION_ENTER);
			_nt_func();
			_log_tree(ACTION_EXIT);
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
			printf("\n");
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
	_log_tree(ACTION_PRINT, "");
	_nt_block();
	_expect(TOK_EOF);
}
