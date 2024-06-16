#include "ast.h"

#include "common/io.h"

#include <assert.h>
#include <string.h>

static void _ast_tree_visualize(ast_node_t *root, unsigned depth) {
	for(unsigned i=0; i<depth; i++) putchar(' ');
	printf("%.*s\n", (int) root->content.size, root->content.string);
	if(root->type < AST_FIRST_LIST_NODE) {
		if(root->children.pair.left != NULL)
			_ast_tree_visualize(root->children.pair.left, depth + 1);
		if(root->children.pair.right != NULL)
			_ast_tree_visualize(root->children.pair.right, depth + 1);
	} else for(size_t i=0; i<root->children.list.count; i++) {
		ast_node_t *child = root->children.list.list[i];
		if(child == NULL) continue;
		_ast_tree_visualize(child, depth + 1);
	}
}

// External Functions //

void ast_tree_visualize(ast_node_t *root) { _ast_tree_visualize(root, 0); }

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
	if(parent->children.list.count == parent->children.list.capacity) {
		ast_node_t *resized = ast_lnode_new(
			tree, parent->children.list.capacity * 2,
			parent->type, parent->content
		);
		resized->children.list.count = parent->children.list.count;
		for(size_t i=0; i<parent->children.list.count; i++)
			resized->children.list.list[i] = parent->children.list.list[i];
		parent = resized;
	}

	size_t child_idx = parent->children.list.count;
	parent->children.list.list[child_idx] = child;
	parent->children.list.count++;
	return parent;
}
