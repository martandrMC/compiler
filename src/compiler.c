#include "common/io.h"
#include "common/arena.h"
#include "lexer/lexer.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>

const char *tok_names[] = {
	"TOK_ERROR", "TOK_EOF",
	"TOK_OPEN_ROUND", "TOK_CLOSE_ROUND",
	"TOK_COMMA", "TOK_COLON", "TOK_SEMICOLON",
	"TOK_OP_ASSIGN", "TOK_OP_PLUS", "TOK_OP_MINUS",
	"TOK_OP_LESS_THAN", "TOK_OP_GREATER_THAN",
	"TOK_KW_DO", "TOK_KW_END", "TOK_KW_RETURN",
	"TOK_KW_VAR", "TOK_KW_IF", "TOK_KW_WHILE",
	"TOK_KW_TRUE", "TOK_KW_FALSE", "TOK_KW_NIL",
	"TOK_KW_AND", "TOK_KW_OR", "TOK_KW_NOT",
	"TOK_TYPE_NAT", "TOK_TYPE_INT", "TOK_TYPE_BOOL",
	"TOK_IDENT", "TOK_LIT_NUM"
};

typedef struct tok_list {
	struct tok_list *next;
	struct token tok;
} tok_list_t;

int main(int argc, char **argv) {
	assert(sizeof(char) == 1);
	if(argc != 2) exit(EXIT_FAILURE);

	lexer_init(argv[1]);
	token_t *bt = lexer_next();

	for(token_t *tok = bt; tok->type != TOK_EOF; tok = lexer_next()) {
		printf("%s(", tok_names[tok->type]);
		str_write(stdout, tok->content);
		puts(")");
	}
	
	puts("================");
	lexer_backtrack(bt);

	for(token_t *tok = lexer_next(); tok->type != TOK_EOF; tok = lexer_next()) {
		printf("%s(", tok_names[tok->type]);
		str_write(stdout, tok->content);
		puts(")");
	}

	exit(EXIT_SUCCESS);
}
