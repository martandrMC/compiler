#ifndef AST_H
#define AST_H

#include "nodes.h"

#include "common/arena.h"
#include "common/io.h"

#define AST_FIRST_LIST_NODE AST_INTERNAL

extern const char *node_type_strs[];
typedef enum ast_node_type {
	FOREACH_NODE(GENERATE_AST_ENUM)
} ast_node_type_t;

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
#define ast_tree_free(tree) (arena_free(tree))
void ast_tree_visualize(ast_node_t *root);

ast_node_t *ast_pnode_new(ast_t *tree, ast_node_type_t type,string_t content);
#define ast_pnode_left(parent,child) (parent->children.pair.left = child)
#define ast_pnode_right(parent,child) (parent->children.pair.right = child)

ast_node_t *ast_lnode_new(ast_t *tree, size_t capacity, ast_node_type_t type, string_t content);
ast_node_t *ast_lnode_add(ast_t *tree, ast_node_t *parent, ast_node_t *child);

#endif // AST_H
