#ifndef AST_H
#define AST_H

#include "common/io.h"
#include "common/arena.h"

#define AST_FIRST_LIST_NODE AST_BLOCK

// TODO
typedef enum ast_node_type {
	// Pair //
	AST_RETURN, AST_WHILE, AST_IF_CASE,
	
	// List //
	AST_BLOCK, AST_VAR, AST_IF_LIST, AST_CALL
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

typedef arena_t ast_t;
#define ast_tree_new() (arena_new(64 * sizeof(ast_node_t)))

ast_node_t *ast_pnode_new(ast_t *tree, ast_node_type_t type,string_t content);
#define ast_pnode_left(parent,child) (parent->children.pair.left = child)
#define ast_pnode_right(parent,child) (parent->children.pair.right = child)

ast_node_t *ast_lnode_new(ast_t *tree, size_t capacity, ast_node_type_t type, string_t content);
ast_node_t *ast_lnode_add(ast_t *tree, ast_node_t *parent, ast_node_t *child);

#endif // AST_H
