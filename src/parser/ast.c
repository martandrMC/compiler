#include "ast.h"

#include "common/io.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#define BOX_CHAR_SIZE 4
static void _ast_tree_visualize(ast_node_t *root, size_t depth, string_t *prefix, bool last) {
	for(size_t i=0; i<depth; i+=4) printf("\x1b[0;32m%.*s ", BOX_CHAR_SIZE, &prefix->string[i]);
	memcpy(&prefix->string[depth - BOX_CHAR_SIZE], last ? "⠀" : "│", BOX_CHAR_SIZE);
	
	printf("\x1b[33m");
	if(root->type == AST_BLOCK) printf("BLOCK\n");
	else printf("%.*s\n", (int) root->content.size, root->content.string);

	if(depth >= prefix->size) {
		char *new_buffer = (char *) realloc(prefix->string, prefix->size * 2);
		error_if(new_buffer == NULL);
		prefix->size *= 2, prefix->string = new_buffer;
	}

	if(root->type < AST_FIRST_LIST_NODE) {
		if(root->children.pair.left != NULL) {
			bool is_last = (root->children.pair.right == NULL);
			char *extra_prefix = (is_last ? "└" : "├");
			memcpy(&prefix->string[depth], extra_prefix, BOX_CHAR_SIZE);
			_ast_tree_visualize(root->children.pair.left, depth + BOX_CHAR_SIZE, prefix, is_last);
		}
		if(root->children.pair.right != NULL) {
			memcpy(&prefix->string[depth], "└", BOX_CHAR_SIZE);
			_ast_tree_visualize(root->children.pair.right, depth + BOX_CHAR_SIZE, prefix, true);
		}
	} else {
		size_t child_count = root->children.list.count;
		for(size_t i=0; i<child_count; i++) {
			bool is_last = (i == child_count - 1);
			char *extra_prefix = (is_last ? "└" : "├");
			memcpy(&prefix->string[depth], extra_prefix, BOX_CHAR_SIZE);

			ast_node_t *child = root->children.list.list[i];
			if(child == NULL) continue;
			_ast_tree_visualize(child, depth + BOX_CHAR_SIZE, prefix, is_last);
		}
	}
}

// External Functions //

#define INITIAL_BUFFER_SIZE 16
void ast_tree_visualize(ast_node_t *root) {
	char *initial_buffer  = (char *) calloc(INITIAL_BUFFER_SIZE, sizeof(char));
	string_t prefix = {.size = INITIAL_BUFFER_SIZE, .string = initial_buffer};
	_ast_tree_visualize(root, 0, &prefix, false); printf("\x1b[0m");
	free(prefix.string);
}

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
