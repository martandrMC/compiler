#include "ast.h"
#include "lookaheads.h"
#include "parser.h"

#include "common/io.h"
#include "lexer/lexer.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "precedences.c"

typedef enum se_variant {
	OUTER_STMT_EXPR,
	DELIM_STMT_EXPR,
	INNER_STMT_EXPR
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

	for(char *c = file_base; c < problem_base; c++)
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
static void _nt_var_expr_next(ast_node_t **parent);
static void _nt_var_stmt_next(ast_node_t **parent);
static ast_node_t *_nt_type(void);
static ast_node_t *_nt_stmt_common(void);
static ast_node_t *_nt_outer_stmt(void);
static ast_node_t *_nt_inner_stmt(void);
static ast_node_t *_nt_stmt_expr(se_variant_t variant);

static ast_node_t *_nt_prec_0(void);
static ast_node_t *_nt_prec_1(void);
static ast_node_t *_nt_prec_2(void);
static ast_node_t *_nt_unaries_2(void);
static ast_node_t *_nt_prec_3(void);
static ast_node_t *_nt_prec_4(void);
static ast_node_t *_nt_prec_5(void);
static ast_node_t *_nt_unaries_5(void);
static ast_node_t *_nt_term(void);
static ast_node_t *_nt_func(ast_node_t *child);

// Internal Function Defs (Non-Terminal Helpers) //

static void _nth_vardecl(ast_node_t **parent) {
	string_t ident_str = _expect(TOK_IDENT)->content;
	string_t assign_str = _expect(TOK_OP_ASSIGN)->content;
	ast_node_t *assign = ast_pnode_new(&ps.ast, AST_OP_BINARY, assign_str);
	*parent = ast_lnode_add(&ps.ast, *parent, assign);
	ast_pnode_left(assign, ast_pnode_new(&ps.ast, AST_IDENT, ident_str));
	ast_pnode_right(assign, _nt_var_init(parent));
}

static ast_node_t *_nth_shunting_yard(void) {
	return ast_pnode_new(&ps.ast, AST_IDENT, TO_STRING("EXPR"));
}

// Internal Functions Defs (Non-Terminals) //

static ast_node_t *_nt_block(void) {
	ast_node_t *node = ast_lnode_new(&ps.ast, 4, AST_BLOCK, EMPTY_STRING);
	loop: switch(PEEK) {
		case TOK_KW_VAR: ;
			ast_node_t *vardecl = ast_lnode_new(&ps.ast, 4, AST_VAR, CONSUME->content);
			vardecl = ast_lnode_add(&ps.ast, vardecl, _nt_type());
			_nth_vardecl(&vardecl);
			node = ast_lnode_add(&ps.ast, node, vardecl);
			goto loop;
		case STMT_FIRSTS:
		case EXPR_FIRSTS:
			node = ast_lnode_add(&ps.ast, node, _nt_stmt_expr(OUTER_STMT_EXPR));
			goto loop;
		case TOK_KW_END:
		case TOK_KW_ELIF:
		case TOK_KW_ELSE:
		case TOK_EOF:
			break;
		default: _panic_expect(CONSUME, "\"var\", statement or expression");
	}
	return node;
}

static ast_node_t *_nt_var_init(ast_node_t **parent) {
	ast_node_t *node = NULL;
	switch(PEEK) {
		case STMT_FIRSTS:
			node = _nt_outer_stmt();
			_nt_var_stmt_next(parent);
			break;
		case EXPR_FIRSTS:
			node = _nt_prec_0();
			_nt_var_expr_next(parent);
			break;
		default: _panic_expect(CONSUME, "statement or expression");
	}
	return node;
}

static void _nt_var_expr_next(ast_node_t **parent) {
	switch(PEEK) {
		case TOK_COMMA: CONSUME;
			_nth_vardecl(parent);
			break;
		case TOK_SEMICOLON: CONSUME;
			break;
		default: _panic_expect(CONSUME, "\",\" or \";\"");
	}
}

static void _nt_var_stmt_next(ast_node_t **parent) {
	switch(PEEK) {
		case TOK_COMMA: CONSUME;
			_nth_vardecl(parent);
			break;
		case TOK_KW_END:
		case TOK_KW_ELIF:
		case TOK_KW_ELSE:
		case TOK_EOF:
		case TOK_KW_VAR:
		case STMT_FIRSTS:
		case EXPR_FIRSTS:
			break;
		default: _panic_expect(CONSUME, "\",\" or \"end\" or EOF or block member");
	}
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
	ast_node_t *node = NULL;
	switch(PEEK) {
		case TOK_KW_IF: ;
			ast_node_t *branch = ast_pnode_new(&ps.ast, AST_IF_CASE, CONSUME->content);
			ast_pnode_left(branch, _nt_stmt_expr(DELIM_STMT_EXPR));
			_expect(TOK_COLON);
			ast_pnode_right(branch, _nt_stmt_expr(INNER_STMT_EXPR));
			node = ast_lnode_new(&ps.ast, 4, AST_IF_LIST, EMPTY_STRING);
			node = ast_lnode_add(&ps.ast, node, branch);
			loop: switch(PEEK) { // BEGIN _nt_else()
				case TOK_KW_ELIF: ;
					branch = ast_pnode_new(&ps.ast, AST_IF_CASE, CONSUME->content);
					ast_pnode_left(branch, _nt_stmt_expr(DELIM_STMT_EXPR));
					_expect(TOK_COLON);
					ast_pnode_right(branch, _nt_stmt_expr(INNER_STMT_EXPR));
					node = ast_lnode_add(&ps.ast, node, branch);
					goto loop;
				case TOK_KW_ELSE: ;
					branch = ast_pnode_new(&ps.ast, AST_IF_CASE, CONSUME->content);
					ast_pnode_left(branch, NULL);
					ast_pnode_right(branch, _nt_stmt_expr(INNER_STMT_EXPR));
					node = ast_lnode_add(&ps.ast, node, branch);
					break;
				case TOK_KW_END:
					break;
				default: _panic_expect(CONSUME, "\"else\" or \"end\"");
			} // END _nt_else()
			_expect(TOK_KW_END);
			break;
		case TOK_KW_WHILE:
			node = ast_pnode_new(&ps.ast, AST_WHILE, CONSUME->content);
			ast_pnode_left(node, _nt_stmt_expr(DELIM_STMT_EXPR));
			_expect(TOK_COLON);
			ast_pnode_right(node, _nt_stmt_expr(INNER_STMT_EXPR));
			_expect(TOK_KW_END);
			break;
		default: _panic_expect(CONSUME, "\"if\" or \"while\"");
	}
	return node;
}

static ast_node_t *_nt_outer_stmt(void) {
	ast_node_t *node = NULL;
	switch(PEEK) {
		case TOK_KW_DO: ;
			string_t content = CONSUME->content;
			node = _nt_block();
			node->content = content;
			_expect(TOK_KW_END);
			break;
		case TOK_KW_RETURN:
			node = ast_pnode_new(&ps.ast, AST_RETURN, CONSUME->content);
			ast_pnode_left(node, _nt_stmt_expr(OUTER_STMT_EXPR));
			ast_pnode_right(node, NULL);
			break;
		case TOK_KW_IF:
		case TOK_KW_WHILE:
			node = _nt_stmt_common();
			break;
		default: _panic_expect(CONSUME, "statement");
	}
	return node;
}

static ast_node_t *_nt_inner_stmt(void) {
	ast_node_t *node = NULL;
	switch(PEEK) {
		case TOK_KW_DO: ;
			string_t content = CONSUME->content;
			node = _nt_block();
			node->content = content;
			break;
		case TOK_KW_RETURN:
			node = ast_pnode_new(&ps.ast, AST_RETURN, CONSUME->content);
			ast_pnode_left(node, _nt_stmt_expr(INNER_STMT_EXPR));
			ast_pnode_right(node, NULL);
			break;
		case TOK_KW_IF:
		case TOK_KW_WHILE:
			node = _nt_stmt_common();
			break;
		default: _panic_expect(CONSUME, "statement");
	}
	return node;
}

static ast_node_t *_nt_stmt_expr(se_variant_t variant) {
	ast_node_t *node = NULL;
	switch(PEEK) {
		case STMT_FIRSTS:
			if(variant == INNER_STMT_EXPR) node = _nt_inner_stmt();
			else node = _nt_outer_stmt();
			break;
		case EXPR_FIRSTS:
			node = _nt_prec_0();
			if(variant == OUTER_STMT_EXPR) _expect(TOK_SEMICOLON);
			break;
		default: _panic_expect(CONSUME, "statement or expression");
	}
	return node;
}

static ast_node_t *_nt_prec_0(void) {
	// Temporary hot-wire for shunting yard rewrite
	ast_node_t *node = NULL;
	switch(PEEK) {
		case TOK_IDENT: CONSUME;
			node = _nth_shunting_yard();
			break;
		default: _panic(CONSUME);
	}
	return node;
	/*
	ast_node_t *node = ast_lnode_new(&ps.ast, 4, AST_INTERNAL, EMPTY_STRING);
	switch(PEEK) {
		case TOK_KW_NOT:
		case TOK_OP_PLUS:
		case TOK_OP_MINUS:
		case TERM_FIRSTS: ;
			node = ast_lnode_add(&ps.ast, node, _nt_prec_1());
			loop: switch(PEEK) { // BEGIN _nt_prec_0_()
				case TOK_OP_ASSIGN:
				case TOK_OP_ASSIGN_ALT: ;
					ast_node_t *operator = ast_pnode_new(&ps.ast, AST_OP_BINARY, CONSUME->content);
					node = ast_lnode_add(&ps.ast, node, operator);
					node = ast_lnode_add(&ps.ast, node, _nt_prec_1());
					goto loop;
				case PREC_0_FOLLOWS:
					break;
				default: _panic(CONSUME);
			} // END _nt_prec_0_()
			for(size_t i = node->children.list.count - 1; i > 1; i -= 2) {
				ast_node_t **left = &node->children.list.list[i - 2];
				ast_node_t *parent = node->children.list.list[i - 1];
				ast_node_t *right = node->children.list.list[i];
				ast_pnode_left(parent, *left);
				ast_pnode_right(parent, right);
				*left = parent;
			}
			node = node->children.list.list[0];
			break;
		default: _panic(CONSUME);
	}
	return node;
	*/
}

static ast_node_t *_nt_prec_1(void) {
	ast_node_t *node = NULL;
	switch(PEEK) {
		case TOK_KW_NOT:
		case TOK_OP_PLUS:
		case TOK_OP_MINUS:
		case TERM_FIRSTS:
			node = _nt_prec_2();
			loop: switch(PEEK) { // BEGIN _nt_prec_1_()
				case TOK_KW_AND:
				case TOK_KW_OR: ;
					ast_node_t *parent = ast_pnode_new(&ps.ast, AST_OP_BINARY, CONSUME->content);
					ast_pnode_left(parent, node);
					ast_pnode_right(parent, _nt_prec_2());
					node = parent;
					goto loop;
				case PREC_1_FOLLOWS:
					break;
				default: _panic(CONSUME);
			} // END _nt_prec_1_()
			break;
		default: _panic(CONSUME);
	}
	return node;
}

static ast_node_t *_nt_prec_2(void) {
	ast_node_t *node = NULL;
	switch(PEEK) {
		case TOK_KW_NOT:
		case TOK_OP_PLUS:
		case TOK_OP_MINUS:
		case TERM_FIRSTS: ;
			node = _nt_unaries_2();
			if(node == NULL) node = _nt_prec_3();
			else ast_pnode_left(node, _nt_prec_3());
			loop: switch(PEEK) { // BEGIN _nt_prec_1_()
				case TOK_OP_COMPARE: ;
					ast_node_t *parent = ast_pnode_new(&ps.ast, AST_OP_BINARY, CONSUME->content);
					ast_pnode_left(parent, node);
					ast_pnode_right(parent, _nt_prec_3());
					node = parent;
					goto loop;
				case PREC_2_FOLLOWS:
					break;
				default: _panic(CONSUME);
			} // END _nt_prec_2_()
			break;
		default: _panic(CONSUME);
	}
	return node;
}

static ast_node_t *_nt_unaries_2(void) {
	ast_node_t *node = NULL;
	switch(PEEK) {
		case TOK_KW_NOT:
			node = ast_pnode_new(&ps.ast, AST_OP_UNARY, CONSUME->content);
			break;
		case TOK_OP_PLUS:
		case TOK_OP_MINUS:
		case TERM_FIRSTS:
			break;
		default: _panic(CONSUME);
	}
	return node;
}

static ast_node_t *_nt_prec_3(void) {
	ast_node_t *node = NULL;
	switch(PEEK) {
		case TOK_OP_PLUS:
		case TOK_OP_MINUS:
		case TERM_FIRSTS:
			node = _nt_prec_4();
			loop: switch(PEEK) { // BEGIN _nt_prec_3_()
				case TOK_OP_PLUS:
				case TOK_OP_MINUS: ;
					ast_node_t *parent = ast_pnode_new(&ps.ast, AST_OP_BINARY, CONSUME->content);
					ast_pnode_left(parent, node);
					ast_pnode_right(parent, _nt_prec_4());
					node = parent;
					goto loop;
				case PREC_3_FOLLOWS:
					break;
				default: _panic(CONSUME);
			} // END _nt_prec_3_()
			break;
		default: _panic(CONSUME);
	}
	return node;
}

static ast_node_t *_nt_prec_4(void) {
	ast_node_t *node = NULL;
	switch(PEEK) {
		case TOK_OP_PLUS:
		case TOK_OP_MINUS:
		case TERM_FIRSTS:
			node = _nt_prec_5();
			loop: switch(PEEK) { // BEGIN _nt_prec_4_()
				case TOK_OP_MULT:
				case TOK_OP_DIV:
				case TOK_OP_MOD: ;
					ast_node_t *parent = ast_pnode_new(&ps.ast, AST_OP_BINARY, CONSUME->content);
					ast_pnode_left(parent, node);
					ast_pnode_right(parent, _nt_prec_5());
					node = parent;
					goto loop;
				case PREC_4_FOLLOWS:
					break;
				default: _panic(CONSUME);
			} // END _nt_prec_4_()
			break;
		default: _panic(CONSUME);
	}
	return node;
}

static ast_node_t *_nt_prec_5(void) {
	ast_node_t *node = NULL;
	switch(PEEK) {
		case TOK_OP_PLUS:
		case TOK_OP_MINUS:
		case TERM_FIRSTS: ;
			node = _nt_unaries_5();
			if(node == NULL) node = _nt_term();
			else ast_pnode_left(node, _nt_term());
			break;
		default: _panic(CONSUME);
	}
	return node;
}

static ast_node_t *_nt_unaries_5(void) {
	ast_node_t *node = NULL;
	switch(PEEK) {
		case TOK_OP_PLUS:
		case TOK_OP_MINUS:
			node = ast_pnode_new(&ps.ast, AST_OP_UNARY, CONSUME->content);
			break;
		case TERM_FIRSTS:
			break;
		default: _panic(CONSUME);
	}
	return node;
}

static ast_node_t *_nt_term(void) {
	ast_node_t *node = NULL;
	switch(PEEK) {
		case TOK_OPEN_ROUND: CONSUME;
			node = _nt_prec_0();
			_expect(TOK_CLOSE_ROUND);
			break;
		case TOK_IDENT:
			node = ast_pnode_new(&ps.ast, AST_IDENT, CONSUME->content);
			switch(PEEK) { // BEGIN _nt_term_()
				case TOK_OPEN_ROUND: CONSUME;
					node = _nt_func(node);
					_expect(TOK_CLOSE_ROUND);
					break;
				case PREC_5_FOLLOWS:
					break;
				default: _panic(CONSUME);
			} // END _nt_term_()
			break;
		case TOK_LIT_NUM:
		case TOK_KW_TRUE:
		case TOK_KW_FALSE:
		case TOK_KW_NIL:
			node = ast_pnode_new(&ps.ast, AST_LITERAL, CONSUME->content);
			break;
		default: _panic(CONSUME);
	}
	return node;
}

static ast_node_t *_nt_func(ast_node_t *child) {
	ast_node_t *node = ast_lnode_new(&ps.ast, 4, AST_CALL, EMPTY_STRING);
	node = ast_lnode_add(&ps.ast, node, child);
	switch(PEEK) {
		case STMT_FIRSTS:
		case EXPR_FIRSTS:
			node = ast_lnode_add(&ps.ast, node, _nt_stmt_expr(DELIM_STMT_EXPR));
			loop: switch(PEEK) { // BEGIN _nt_func_()
				case TOK_COMMA: CONSUME;
					node = ast_lnode_add(&ps.ast, node, _nt_stmt_expr(DELIM_STMT_EXPR));
					goto loop;
				case TOK_CLOSE_ROUND:
					break;
				default: _panic(CONSUME);
			}
			break; // END _nt_func_()
		case TOK_CLOSE_ROUND:
			break;
		default: _panic(CONSUME);
	}
	return node;
}

// External Functions //

void parser_start(void) {
	ps.ast = ast_tree_new();
	ast_node_t *root = NULL;
	switch(PEEK) {
		case TOK_KW_VAR:
		case STMT_FIRSTS:
		case EXPR_FIRSTS:
			root = _nt_block();
			_expect(TOK_EOF);
		case TOK_EOF:
			break;
		default: _panic_expect(CONSUME, "\"var\" statement or expression or EOF");
	}
	if(root == NULL) printf("The file is empty.\n");
	else ast_tree_visualize(root);
	ast_tree_free(&ps.ast);
}
