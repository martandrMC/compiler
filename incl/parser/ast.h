#ifndef AST_H
#define AST_H

#include "common/io.h"
#include "common/arena.h"

#define AST_FIRST_LIST_NODE AST_BLOCK

// TODO
typedef enum ast_node_type {
	// Pair //
	AST_RETURN, AST_WHILE,

	// List //
	AST_BLOCK, AST_CALL
} ast_node_type_t;


// UNTESTED
#pragma GCC diagnostic push 
#pragma GCC diagnostic ignored "-Wpedantic"
typedef struct ast_node {
	ast_node_type_t type;
	string_t content;
	union {
		struct {
			struct ast_node *left;
			struct ast_node *right;
		} pair;
		struct {
			size_t count;
			size_t capacity;
			struct ast_node *list[];
		} list;
	} children;
} ast_node_t;
#pragma GCC diagnostic pop

arena_t ast_new_tree(void);

ast_node_t *ast_new_pair_node(
	arena_t *tree, ast_node_type_t type,
	string_t content
);

ast_node_t *ast_new_list_node(
	arena_t *tree, size_t initial_capacity,
	ast_node_type_t type, string_t content
);

#endif // AST_H
