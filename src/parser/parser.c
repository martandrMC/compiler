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
	OUTER_STMT_EXPR,
	DELIM_STMT_EXPR,
	INNER_STMT_EXPR,
} se_variant_t;

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
static ast_node_t *_nt_var_init(ast_node_t **parent);
static ast_node_t *_nt_var_expr_next(ast_node_t **parent);
static ast_node_t *_nt_var_stmt_next(ast_node_t **parent);
static ast_node_t *_nt_type(void);
static ast_node_t *_nt_stmt_common(void);
static ast_node_t *_nt_else(void);
static ast_node_t *_nt_outer_stmt(void);
static ast_node_t *_nt_inner_stmt(void);
static ast_node_t *_nt_outer_stmt_expr(void);
static ast_node_t *_nt_delim_stmt_expr(void);
static ast_node_t *_nt_inner_stmt_expr(void);

static ast_node_t *_nt_prec_0(void);
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
		case TOK_KW_VAR: ;
			ast_node_t *vardecl = ast_lnode_new(&ps.ast, 4, AST_VAR, CONSUME->content);
			vardecl = ast_lnode_add(&ps.ast, vardecl, _nt_type());
			string_t ident_str = _expect(TOK_IDENT)->content;
			string_t assign_str = _expect(TOK_OP_ASSIGN)->content;
			ast_node_t *assign = ast_pnode_new(&ps.ast, AST_OP_BINARY, assign_str);
			ast_pnode_left(assign, ast_pnode_new(&ps.ast, AST_IDENT, ident_str));
			ast_pnode_right(assign, _nt_var_init(&vardecl));
			block = ast_lnode_add(&ps.ast, block, vardecl);
			goto loop;
		case STMT_FIRSTS:
		case EXPR_FIRSTS:
			_nt_outer_stmt_expr();
			goto loop;
		case TOK_KW_END:
		case TOK_KW_ELSE:
		case TOK_EOF:
			break;
		default: _panic_expect(CONSUME, "\"var\", statement or expression");
	}
	return block;
}

static ast_node_t *_nt_var_init(ast_node_t **parent) {
	ast_node_t *value = NULL;
	switch(PEEK) {
		case STMT_FIRSTS:
			_nt_outer_stmt();
			_nt_var_stmt_next(parent);
			break;
		case EXPR_FIRSTS:
			_nt_prec_0();
			_nt_var_expr_next(parent);
			break;
		default: _panic_expect(CONSUME, "statement or expression");
	}
	return value;
}

static ast_node_t *_nt_var_expr_next(ast_node_t **parent) {
	switch(PEEK) {
		case TOK_COMMA: CONSUME;
			_expect(TOK_IDENT);
			_expect(TOK_OP_ASSIGN);
			_nt_var_init(parent);
			break;
		case TOK_SEMICOLON: CONSUME;
			break;
		default: _panic_expect(CONSUME, "\",\" or \";\"");
	}
	return NULL; // Placeholder
}

static ast_node_t *_nt_var_stmt_next(ast_node_t **parent) {
	switch(PEEK) {
		case TOK_COMMA: CONSUME;
			_expect(TOK_IDENT);
			_expect(TOK_OP_ASSIGN);
			_nt_var_init(parent);
			break;
		case TOK_KW_END:
		case TOK_KW_ELSE:
		case TOK_EOF:
		case TOK_KW_VAR:
		case STMT_FIRSTS:
		case EXPR_FIRSTS:
			break;
		default: _panic_expect(CONSUME, "\",\" or \"end\" or EOF or block member");
	}
	return NULL; // Placeholder
}

static ast_node_t *_nt_type(void) {
	switch(PEEK) {
		case TOK_TYPE_NAT:
		case TOK_TYPE_INT:
		case TOK_TYPE_BOOL:
			return ast_pnode_new(&ps.ast, AST_TYPE, CONSUME->content);
		default: _panic_expect(CONSUME, "\"nat\" or \"int\" or \"bool\"");
	}
	return NULL;
}

static ast_node_t *_nt_stmt_common(void) {
	switch(PEEK) {
		case TOK_KW_IF: CONSUME;
			_nt_delim_stmt_expr();
			_expect(TOK_COLON);
			_nt_inner_stmt_expr();
			_nt_else();
			_expect(TOK_KW_END);
			break;
		case TOK_KW_WHILE: CONSUME;
			_nt_delim_stmt_expr();
			_expect(TOK_COLON);
			_nt_inner_stmt_expr();
			_expect(TOK_KW_END);
			break;
		default: _panic_expect(CONSUME, "\"if\" or \"while\"");
	}
	return NULL; // Placeholder
}

static ast_node_t *_nt_else(void) {
	switch(PEEK) {
		case TOK_KW_ELSE: CONSUME;
			return _nt_inner_stmt_expr();
		case TOK_KW_END:
			break;
		default: _panic_expect(CONSUME, "\"else\" or \"end\"");
	}
	return NULL;
}

static ast_node_t *_nt_outer_stmt(void) {
	switch(PEEK) {
		case TOK_KW_DO: CONSUME;
			ast_node_t *tmp = _nt_block();
			_expect(TOK_KW_END);
			return tmp;
		case TOK_KW_RETURN: CONSUME;
			return _nt_outer_stmt_expr();
		case TOK_KW_IF:
		case TOK_KW_WHILE:
			return _nt_stmt_common();
		default: _panic_expect(CONSUME, "statement");
	}
	return NULL;
}

static ast_node_t *_nt_inner_stmt(void) {
	switch(PEEK) {
		case TOK_KW_DO: CONSUME;
			return _nt_block();
		case TOK_KW_RETURN: CONSUME;
			return _nt_inner_stmt_expr();
		case TOK_KW_IF:
		case TOK_KW_WHILE:
			return _nt_stmt_common();
		default: _panic_expect(CONSUME, "statement");
	}
	return NULL;
}

static ast_node_t *_nt_outer_stmt_expr(void) {
	switch(PEEK) {
		case STMT_FIRSTS:
			return _nt_outer_stmt();
		case EXPR_FIRSTS: ;
			ast_node_t *tmp = _nt_prec_0();
			_expect(TOK_SEMICOLON);
			return tmp;
		default: _panic_expect(CONSUME, "statement or expression");
	}
	return NULL;
}

static ast_node_t *_nt_delim_stmt_expr(void) {
	switch(PEEK) {
		case STMT_FIRSTS:
			return _nt_outer_stmt();
		case EXPR_FIRSTS:
			return _nt_prec_0();
		default: _panic_expect(CONSUME, "statement or expression");
	}
	return NULL;
}

static ast_node_t *_nt_inner_stmt_expr(void) {
	switch(PEEK) {
		case STMT_FIRSTS:
			return _nt_inner_stmt();
		case EXPR_FIRSTS:
			return _nt_prec_0();
		default: _panic_expect(CONSUME, "statement or expression");
	}
	return NULL;
}

static ast_node_t *_nt_prec_0(void) {
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
	return NULL; // Placeholder
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
			_nt_delim_stmt_expr();
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
			_nt_delim_stmt_expr();
			goto loop;
		case TOK_CLOSE_ROUND:
			break;
		default: _panic(CONSUME);
	}
}

// External Functions //

void parser_start(void) {
	ps.ast = ast_tree_new();
	switch(PEEK) {
		case TOK_KW_VAR:
		case STMT_FIRSTS:
		case EXPR_FIRSTS:
			_nt_block();
		case TOK_EOF:
			break;
		default: _panic_expect(CONSUME, "\"var\" statement or expression or EOF");
	}
}
