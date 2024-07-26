#include "scope.h"

#include <assert.h>
#include <stdbool.h>

void scope_walker(ast_node_t *current) {
	if(current == NULL) return;
	bool flag = false;
	switch(current->type) {
		case AST_RETURN:
		case AST_WHILE:
		case AST_IF_SINGLE:
			scope_walker(current->children.pair.left);
			scope_walker(current->children.pair.right);
			break;
		case AST_BLOCK:
			// special case for block
			flag = true;
			printf("->\n");
			// fall through
		case AST_CALL:
		case AST_IF_LIST:
		case AST_VAR_LIST:
			for(size_t i=0; i<current->children.list.count; i++)
				scope_walker(current->children.list.list[i]);
			break;
		case AST_VAR_SINGLE:
			printf("%.*s\n",
				(int)current->content.size,
				current->content.string
			);
		default: return;
	}
	if(flag) printf("<-\n");
}

void scope_run(ast_t *ast) {
	assert(ast->root->type == AST_BLOCK);
	scope_walker(ast->root);
}
