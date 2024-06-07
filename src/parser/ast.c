#include "ast.h"

#include "common/io.h"
#include "common/arena.h"

#include <assert.h>
#include <string.h>

arena_t ast_new_tree(void) {
	return arena_new(64 * sizeof(ast_node_t));
}

ast_node_t *ast_new_pair_node(arena_t *tree, ast_node_type_t type, string_t content) {
	assert(type < AST_FIRST_LIST_NODE);
	ast_node_t *node = (ast_node_t *) arena_alloc(tree, sizeof(ast_node_t));
	error_if(node == NULL);
	node->type = type, node->content = content;
	node->children.pair.left = node->children.pair.right = NULL; // redundant
	return node;
}

ast_node_t *ast_new_list_node(
	arena_t *tree, size_t initial_capacity,
	ast_node_type_t type, string_t content
) {
	assert(type >= AST_FIRST_LIST_NODE);
	size_t list_size_bytes = initial_capacity * sizeof(ast_node_t *);
	ast_node_t *node = (ast_node_t *) arena_alloc(tree, sizeof(ast_node_t) + list_size_bytes);
	error_if(node == NULL);
	node->type = type, node->content = content;
	node->children.list.capacity = initial_capacity;
	node->children.list.count = 0; // redundant
	memset(node->children.list.list, 0, list_size_bytes); // redundant
	return node;
}
