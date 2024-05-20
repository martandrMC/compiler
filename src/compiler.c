#include "io.h"
#include "lexer.h"
#include "arena.h"

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

	FILE *fdesc = fopen(argv[1], "r");
	error_if(!fdesc);
	string_t file = str_read(fdesc);
	error_if(!file.string);
	fclose(fdesc);

	arena_t toks = arena_new(10 * sizeof (tok_list_t));
	tok_list_t *prev = NULL, *list;

	for(
		token_t tok = init_lexer(file);
		tok.type != TOK_EOF;
		tok = next_token()
	) {
		tok_list_t *curr = arena_alloc(&toks, sizeof (tok_list_t));
		curr->tok = tok, curr->next = NULL;
		if(prev != NULL) prev->next = curr;
		else list = curr;
		prev = curr;
	}

	for(tok_list_t *curr = list; curr != NULL; curr = curr->next) {
		token_t tok = curr->tok;
		printf("%s(", tok_names[tok.type]);
		str_write(stdout, tok.content);
		puts(")");
	}

	puts("And again!");

	for(tok_list_t *curr = list; curr != NULL; curr = curr->next) {
		token_t tok = curr->tok;
		printf("%s(", tok_names[tok.type]);
		str_write(stdout, tok.content);
		puts(")");
	}

	arena_free(&toks);
	free(file.string);

	exit(EXIT_SUCCESS);
}
