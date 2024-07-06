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

static struct parser_state {
	ast_t ast;
} ps;

// Internal Functions (Helpers) //

static void panic_common(token_t *problem) {
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

static void panic(token_t *problem, const char *message) {
	panic_common(problem);
	if(message != NULL) printf(", expected: %s\n", message);
	else printf("\n");
	exit(EXIT_FAILURE);
}

static token_t *expect(token_type_t type) {
	token_t *next = CONSUME;
	if(next->type != type) {
		panic_common(next);
		printf(", expected: %s\n", token_type_strs[type]);
		exit(EXIT_FAILURE);
	}
	return next;
}

// Internal Function Decls (Non-Terminals) //

static ast_node_t *parse_block(void);
static ast_node_t *parse_type(void);
static ast_node_t *parse_statement(bool inner);

static ast_node_t *parse_expression(arena_t *reused_arena);
static ast_node_t *parse_term(arena_t *reused_arena);

// Internal Function Defs (Non-Terminal Helpers) //

typedef struct operator {
	token_t *token;
	unsigned prec;
	bool unary;
	bool left;
} operator_t;

static operator_t sy_get_binop(void) {
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
		default: panic(token, "a valid binary operator");
	}
	return ret;
}

static operator_t sy_get_unop(void) {
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
		default: panic(CONSUME, "a valid unary operator");
	}
	return ret;
}

static void sy_pop_operator(vector_t **output, vector_t **opstack) {
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

static ast_node_t *shunting_yard(arena_t *arena) {
	bool atom = true;
	vector_t *output = vector_new(arena, sizeof(ast_node_t *), 16);
	vector_t *opstack = vector_new(arena, sizeof(operator_t), 16);

	while(true) {
		switch(PEEK) {
			case EXPR_FOLLOWS: goto exit;
			default: ;
		}

		if(atom) {
			operator_t new_op = sy_get_unop();
			if(new_op.prec == 0) {
				ast_node_t *new_atom = parse_term(arena);
				vector_add(&output, &new_atom);
				atom = false;
			} else vector_add(&opstack, &new_op);
		} else {
			operator_t new_op = sy_get_binop();
			while(opstack->count > 0 && (new_op.left ?
				((operator_t *) vector_peek(opstack))->prec < new_op.prec :
				((operator_t *) vector_peek(opstack))->prec <= new_op.prec
			)) sy_pop_operator(&output, &opstack);
			vector_add(&opstack, &new_op);
			atom = true;
		}
	} exit: ;

	if(atom) panic(CONSUME, "another expression term");
	for(size_t i=0, size = opstack->count; i<size; i++)
		sy_pop_operator(&output, &opstack);

	if(output->count != 1) panic(CONSUME, "a well-formed expression");
	ast_node_t *expr; vector_take(output, &expr);
	return expr;
}

static ast_node_t *statement_or_expression(bool inner_stmt, bool sem_expr) {
	ast_node_t *node = NULL;
	switch(PEEK) {
		case STMT_FIRSTS:
			node = parse_statement(inner_stmt);
			break;
		case EXPR_FIRSTS:
			node = parse_expression(NULL);
			if(sem_expr) expect(TOK_SEMICOLON);
			break;
		default: panic(CONSUME, "statement or expression");
	}
	return node;
}

static ast_node_t *enclosed_block(bool with_end) {
	string_t content = expect(TOK_KW_DO)->content;
	ast_node_t *node = parse_block();
	node->content = content;
	if(with_end) expect(TOK_KW_END);
	return node;
}

static ast_node_t *statement_content(bool with_end) {
	ast_node_t *node = NULL;
	switch(PEEK) {
		case TOK_COLON: CONSUME;
			node = statement_or_expression(true, false);
			if(with_end) expect(TOK_KW_END);
			break;
		case TOK_KW_DO:
			node = enclosed_block(true);
			break;
		default: panic(CONSUME, "\":\" or inline block");
	}
	return node;
}

// Internal Functions Defs (Non-Terminals) //

static ast_node_t *parse_block(void) {
	ast_node_t *node = ast_lnode_new(&ps.ast, 4, AST_BLOCK, EMPTY_STRING);
	while(true) switch(PEEK) {
		case STMT_FIRSTS:
		case EXPR_FIRSTS:
			node = ast_lnode_add(&ps.ast, node, statement_or_expression(false, true));
			break;
		case TOK_KW_VAR: ;
			ast_node_t *varlist = ast_lnode_new(&ps.ast, 4, AST_VAR_LIST, CONSUME->content);
			while(true) {
				string_t identifier = expect(TOK_IDENT)->content;
				ast_node_t *variable = ast_pnode_new(&ps.ast, AST_VAR_SINGLE, identifier);
				ast_pnode_left(variable, parse_type());
				expect(TOK_OP_ASSIGN);

				bool expr;
				switch(PEEK) {
					case EXPR_FIRSTS: expr = true; break;
					default: expr = false; break;
				}

				ast_pnode_right(variable, statement_or_expression(false, false));
				varlist = ast_lnode_add(&ps.ast, varlist, variable);

				if(PEEK != TOK_COMMA) {
					if(expr) expect(TOK_SEMICOLON);
					break;
				} else CONSUME;
			}
			node = ast_lnode_add(&ps.ast, node, varlist);
		case TOK_EOF:
		case TOK_KW_END:
			goto exit;
		default: panic(CONSUME, "a statement or an expression");
	} exit: ;
	return node;
}

static ast_node_t *parse_type(void) {
	if(PEEK != TOK_COLON) return NULL;
	CONSUME;
	switch(PEEK) {
		case TOK_TYPE_NAT:
		case TOK_TYPE_INT:
		case TOK_TYPE_BOOL:
			return ast_pnode_new(&ps.ast, AST_TYPE, CONSUME->content);
		default: panic(CONSUME, "a valid type");
	}
	return NULL;
}

static ast_node_t *parse_statement(bool inner_stmt) {
	ast_node_t *node = NULL;
	switch(PEEK) {
		case TOK_KW_DO:
			node = enclosed_block(true);
			break;
		case TOK_KW_RETURN:
			node = ast_pnode_new(&ps.ast, AST_RETURN, CONSUME->content);
			ast_pnode_left(node, statement_or_expression(inner_stmt, !inner_stmt));
			break;
		case TOK_KW_WHILE:
			node = ast_pnode_new(&ps.ast, AST_WHILE, CONSUME->content);
			ast_pnode_left(node, statement_or_expression(true, false));
			ast_pnode_right(node, statement_content(true));
			break;
		case TOK_KW_IF:
			node = ast_lnode_new(&ps.ast, 4, AST_IF_LIST, EMPTY_STRING);
			for(bool else_next = false; ; ) {
				ast_node_t *branch = ast_pnode_new(&ps.ast, AST_IF_SINGLE, CONSUME->content);
				if(!else_next) ast_pnode_left(branch, statement_or_expression(true, false));
				ast_pnode_right(branch, statement_content(false));
				node = ast_lnode_add(&ps.ast, node, branch);

				if(else_next) break;
				token_type_t next = PEEK;
				if(next == TOK_KW_ELSE) else_next = true;
				else if(next != TOK_KW_ELIF) break;
			}
			expect(TOK_KW_END);
			break;
		default: panic(CONSUME, "a statement");
	}
	return node;
}

static ast_node_t *parse_expression(arena_t *reused_arena) {
	arena_t *arena;
	if(reused_arena == NULL) {
		arena_t new_arena = arena_new(1024);
		arena = &new_arena;
	} else arena = reused_arena;
	ast_node_t *node = shunting_yard(arena);
	if(reused_arena == NULL) arena_free(arena);
	return node;
}

static ast_node_t *parse_term(arena_t *reused_arena) {
	ast_node_t *node = NULL;
	switch(PEEK) {
		case TOK_LIT_NUM:
		case TOK_KW_TRUE:
		case TOK_KW_FALSE:
		case TOK_KW_NIL:
			node = ast_pnode_new(&ps.ast, AST_LITERAL, CONSUME->content);
			break;
		case TOK_OPEN_ROUND: CONSUME;
			node = parse_expression(reused_arena);
			expect(TOK_CLOSE_ROUND);
			break;
		case TOK_IDENT: ;
			string_t content = CONSUME->content;
			if(PEEK == TOK_OPEN_ROUND) { CONSUME;
				node = ast_lnode_new(&ps.ast, 4, AST_CALL, content);
				if(PEEK != TOK_CLOSE_ROUND) while(true) {
					node = ast_lnode_add(&ps.ast, node, statement_or_expression(true, false));
					if(PEEK != TOK_CLOSE_ROUND) expect(TOK_COMMA);
					else break;
				}
			} else node = ast_pnode_new(&ps.ast, AST_IDENT, content);
			break;
		default: panic(CONSUME, "an expression term.");
	}
	return node;
}

// External Functions //

void parser_start(void) {
	ps.ast = ast_tree_new();
	ast_node_t *root = parse_block();
	expect(TOK_EOF);
	ast_tree_visualize(root);
	ast_tree_free(&ps.ast);
}
