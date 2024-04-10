#include "io.h"
#include "lexer.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>

const char *tok_names[] = {
	"TOK_ERROR", "TOK_EOF",
	"TOK_OPEN_ROUND", "TOK_CLOSE_ROUND",
	"TOK_OPEN_CURLY", "TOK_CLOSE_CURLY",
	"TOK_COMMA", "TOK_SEMICOLON",
	"TOK_ASSIGN", "TOK_PLUS", "TOK_LESS_THAN",
	"TOK_KW_VAR", "TOK_KW_IF", "TOK_KW_LOOP",
	"TOK_KW_TRUE", "TOK_KW_FALSE",
	"TOK_TYPE_INT", "TOK_TYPE_BOOL",
	"TOK_IDENT", "TOK_LIT_INT"
};

int main(int argc, char **argv) {
	assert(sizeof(char) == 1);
	if(argc != 2) exit(EXIT_FAILURE);

	FILE *fdesc = fopen(argv[1], "r");
	error_if(!fdesc);
	string_t file = str_read(fdesc);
	error_if(!file.string);
	fclose(fdesc);

	for(
		token_t tok = init_lexer(file);
		tok.type != TOK_EOF;
		tok = next_token()
	) {
		printf("%s(", tok_names[tok.type]);
		str_write(stdout, tok.content);
		puts(")");
	}

	exit(EXIT_SUCCESS);
}
