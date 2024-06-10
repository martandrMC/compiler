#include "ast.h"

#include "common/io.h"

#include <assert.h>
#include <string.h>

ast_node_t *ast_pnode_new(ast_t *tree, ast_node_type_t type, string_t content) {
	assert(type < AST_FIRST_LIST_NODE);
	ast_node_t *node = (ast_node_t *) arena_alloc(tree, sizeof(ast_node_t));
	error_if(node == NULL);
	node->type = type, node->content = content;
	node->children.pair.left = node->children.pair.right = NULL; // redundant
	return node;
}

ast_node_t *ast_lnode_new(ast_t *tree, size_t capacity, ast_node_type_t type, string_t content) {
	assert(type >= AST_FIRST_LIST_NODE);
	size_t list_size_bytes = capacity * sizeof(ast_node_t *);
	ast_node_t *node = (ast_node_t *) arena_alloc(tree, sizeof(ast_node_t) + list_size_bytes);
	error_if(node == NULL);
	node->type = type, node->content = content;
	node->children.list.capacity = capacity;
	node->children.list.count = 0; // redundant
	memset(node->children.list.list, 0, list_size_bytes); // redundant
	return node;
}

ast_node_t *ast_lnode_add(ast_t *tree, ast_node_t *parent, ast_node_t *child) {
	(void) tree; // TODO: Realloc the parent when child list overflows
	assert(parent->children.list.count < parent->children.list.capacity);

	size_t child_idx = parent->children.list.count;
	parent->children.list.list[child_idx] = child;
	parent->children.list.count++;
	return parent;
}
