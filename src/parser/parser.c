#include "ast.h"
#include "lookaheads.h"
#include "parser.h"

#include "common/io.h"
#include "common/vector.h"
#include "lexer/lexer.h"

#include <stdio.h>
#include <stdlib.h>

#define PEEK lexer_peek()->type
#define CONSUME lexer_next()

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

static void _panic(token_t *problem, const char *message) {
	_panic_common(problem);
	if(message != NULL) printf(", expected: %s\n", message);
	else printf("\n");
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

static ast_node_t *_nt_expr(arena_t *reused_arena);
static ast_node_t *_nt_term(arena_t *reused_arena);
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

typedef struct operator {
	token_t *token;
	unsigned prec;
	bool unary;
	bool left;
} operator_t;

static operator_t _nth_get_binary_operator(void) {
	token_t *token = CONSUME;
	operator_t ret = {.token = token, .prec = 0, .unary = false, .left = false};
	switch(token->type) {
		case TOK_OP_ASSIGN:
		case TOK_OP_ASSIGN_ALT:
			ret.prec = 5;
			ret.left = true;
			break;
		case TOK_KW_AND:
		case TOK_KW_OR:
			ret.prec = 4;
			break;
		case TOK_OP_COMPARE:
			ret.prec = 3;
			break;
		case TOK_OP_PLUS:
		case TOK_OP_MINUS:
			ret.prec = 2;
			break;
		case TOK_OP_MULT:
		case TOK_OP_DIV:
		case TOK_OP_MOD:
			ret.prec = 1;
			break;
		default: _panic(token, "a valid binary operator");
	}
	return ret;
}

static operator_t _nth_get_unary_operator(void) {
	//token_t *token = CONSUME;
	operator_t ret = {.token = NULL, .prec = 0, .unary = true, .left = true};
	switch(PEEK) {
		case TOK_KW_NOT:
			ret.token = CONSUME;
			ret.prec = 4;
			break;
		case TOK_OP_PLUS:
		case TOK_OP_MINUS:
			ret.token = CONSUME;
			ret.prec = 1;
			break;
		case TERM_FIRSTS:
			break;
		default: _panic(CONSUME, "a valid unary operator");
	}
	return ret;
}

static void _nth_pop_operator(vector_t **output, vector_t **opstack) {
	operator_t old_op; vector_take(*opstack, &old_op);
	ast_node_type_t node_type = old_op.unary ? AST_OP_UNARY : AST_OP_BINARY;
	ast_node_t *node = ast_pnode_new(&ps.ast, node_type, old_op.token->content);

	ast_node_t *tmp = NULL;
	vector_take(*output, &tmp);
	ast_pnode_right(node, tmp);
	if(!old_op.unary) {
		vector_take(*output, &tmp);
		ast_pnode_left(node, tmp);
	}

	vector_add(output, &node);
}

static ast_node_t *_nth_shunting_yard(arena_t *arena) {
	bool atom = true;
	vector_t *output = vector_new(arena, sizeof(ast_node_t *), 16);
	vector_t *opstack = vector_new(arena, sizeof(operator_t), 16);

	while(true) {
		switch(PEEK) {
			case EXPR_FOLLOWS: goto exit;
			default: ;
		}

		if(atom) {
			operator_t new_op = _nth_get_unary_operator();
			if(new_op.prec == 0) {
				ast_node_t *new_atom = _nt_term(arena);
				vector_add(&output, &new_atom);
				atom = false;
			} else vector_add(&opstack, &new_op);
		} else {
			operator_t new_op = _nth_get_binary_operator();
			while(opstack->count > 0 && (new_op.left ?
				((operator_t *) vector_peek(opstack))->prec < new_op.prec :
				((operator_t *) vector_peek(opstack))->prec <= new_op.prec
			)) _nth_pop_operator(&output, &opstack);
			vector_add(&opstack, &new_op);
			atom = true;
		}
	} exit: ;

	if(atom) _panic(CONSUME, "another expression term");
	for(size_t i=0, size = opstack->count; i<size; i++)
		_nth_pop_operator(&output, &opstack);

	if(output->count != 1) _panic(CONSUME, "a well-formed expression");
	ast_node_t *expr; vector_take(output, &expr);
	return expr;
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
		default: _panic(CONSUME, "\"var\", statement or expression");
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
			node = _nt_expr(NULL);
			_nt_var_expr_next(parent);
			break;
		default: _panic(CONSUME, "statement or expression");
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
		default: _panic(CONSUME, "\",\" or \";\"");
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
		default: _panic(CONSUME, "\",\" or \"end\" or EOF or block member");
	}
}

static ast_node_t *_nt_type(void) {
	switch(PEEK) {
		case TOK_TYPE_NAT:
		case TOK_TYPE_INT:
		case TOK_TYPE_BOOL:
			return ast_pnode_new(&ps.ast, AST_TYPE, CONSUME->content);
		default: _panic(CONSUME, "\"nat\" or \"int\" or \"bool\"");
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
				default: _panic(CONSUME, "\"else\" or \"end\"");
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
		default: _panic(CONSUME, "\"if\" or \"while\"");
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
		default: _panic(CONSUME, "statement");
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
		default: _panic(CONSUME, "statement");
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
			node = _nt_expr(NULL);
			if(variant == OUTER_STMT_EXPR) _expect(TOK_SEMICOLON);
			break;
		default: _panic(CONSUME, "statement or expression");
	}
	return node;
}

static ast_node_t *_nt_expr(arena_t *reused_arena) {
	arena_t *arena;
	if(reused_arena == NULL) {
		arena_t new_arena = arena_new(1024);
		arena = &new_arena;
	} else arena = reused_arena;

	ast_node_t *node = NULL;
	switch(PEEK) {
		case EXPR_FIRSTS:
			node = _nth_shunting_yard(arena);
			break;
		default: _panic(CONSUME, "a valid unary operator or term");
	}

	if(reused_arena == NULL) arena_free(arena);
	return node;
}

static ast_node_t *_nt_term(arena_t *reused_arena) {
	ast_node_t *node = NULL;
	switch(PEEK) {
		case TOK_OPEN_ROUND: CONSUME;
			node = _nt_expr(reused_arena);
			_expect(TOK_CLOSE_ROUND);
			break;
		case TOK_IDENT:
			node = ast_pnode_new(&ps.ast, AST_IDENT, CONSUME->content);
			switch(PEEK) { // BEGIN _nt_term_()
				case TOK_OPEN_ROUND: CONSUME;
					node = _nt_func(node);
					_expect(TOK_CLOSE_ROUND);
					break;
				case TERM_FOLLOWS:
					break;
				default: _panic(CONSUME, "a function call");
			} // END _nt_term_()
			break;
		case TOK_LIT_NUM:
		case TOK_KW_TRUE:
		case TOK_KW_FALSE:
		case TOK_KW_NIL:
			node = ast_pnode_new(&ps.ast, AST_LITERAL, CONSUME->content);
			break;
		default: _panic(CONSUME, "a literal, an identifier, or a subexpression.");
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
				default: _panic(CONSUME, "\",\" or \")\"");
			}
			break; // END _nt_func_()
		case TOK_CLOSE_ROUND:
			break;
		default: _panic(CONSUME, "a valid function argument");
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
		default: _panic(CONSUME, "\"var\" statement or expression or EOF");
	}
	if(root == NULL) printf("The file is empty.\n");
	else ast_tree_visualize(root);
	ast_tree_free(&ps.ast);
}
