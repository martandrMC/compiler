#include "parser.h"
#include "lookaheads.h"
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
static void _nt_var_init_(void);
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
static void _nt_unaries_2(void);
static void _nt_prec_3(void);
static void _nt_prec_3_(void);
static void _nt_prec_4(void);
static void _nt_prec_4_(void);
static void _nt_prec_5(void);
static void _nt_unaries_5(void);
static void _nt_term(void);
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
			_nt_var_init();
			goto loop;
			break;
		case STMT_FIRSTS:
		case EXPR_FIRSTS:
			_nt_stmt_expr_sem();
			goto loop;
			break;
		case TOK_KW_END:
		case TOK_EOF:
			_log_tree(ACTION_EXIT);
			break;
		default: _panic(CONSUME);
	}
}

static void _nt_var_init(void) {
	switch(PEEK) {
		case TOK_OP_ASSIGN: CONSUME;
			printf(" = ");
			_nt_var_init_();
			break;
		case TOK_COMMA:
		case TOK_KW_END:
		case TOK_EOF:
		case STMT_FIRSTS:
		case EXPR_FIRSTS:
			printf("\n");
			_nt_var_stmt_next();
			break;
		default: _panic(CONSUME);
	}
}

static void _nt_var_init_(void) {
	switch(PEEK) {
		case STMT_FIRSTS:
			_nt_stmt();
			_nt_var_stmt_next();
			break;
		case EXPR_FIRSTS:
			_nt_prec_0();
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
	printf("EXPR\n");
	_log_tree(ACTION_ENTER);
	switch(PEEK) {
		case TOK_IDENT: ; // C being C
			token_t *id = CONSUME;
			_log_tree(ACTION_PRINT, "%.*s\n",
				id->content.size,
				id->content.string
			);
			break;
		default: _panic(CONSUME);
	}
	_log_tree(ACTION_EXIT);
}
static void _nt_prec_0_(void) {}

static void _nt_prec_1(void) {}
static void _nt_prec_1_(void) {}

static void _nt_prec_2(void) {}
static void _nt_prec_2_(void) {}
static void _nt_unaries_2(void) {}

static void _nt_prec_3(void) {}
static void _nt_prec_3_(void) {}

static void _nt_prec_4(void) {}
static void _nt_prec_4_(void) {}

static void _nt_prec_5(void) {}
static void _nt_unaries_5(void) {}

static void _nt_term(void) {}

static void _nt_func(void) {}
static void _nt_func_(void) {}

// External Functions //

void parser_start(void) {
	_log_tree(ACTION_PRINT, "");
	_nt_block();
	_expect(TOK_EOF);
}
