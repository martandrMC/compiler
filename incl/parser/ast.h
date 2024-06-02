#ifndef AST_H
#define AST_H

#include "common/io.h"

typedef enum ast_node_type {
	// TODO
} ast_node_type_t;

typedef struct ast_node {
	ast_node_type_t type;
	string_t content;
	unsigned child_count;
	unsigned child_capacity;
	struct ast_node *children[];
} ast_node_t;

#endif // AST_H
