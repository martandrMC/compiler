#include "common/io.h"
#include "lexer/lexer.h"
#include "parser/parser.h"

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

int main(int argc, char **argv) {
	assert(sizeof(char) == 1);
	if(argc != 2) exit(EXIT_FAILURE);

	lexer_init(argv[1]);
	token_t *bt = lexer_next();

	for(token_t *tok = bt; tok->type != TOK_EOF; tok = lexer_next()) {
		printf("%s(", token_type_strs[tok->type]);
		str_write(stdout, tok->content);
		puts(")");
	}

	parser_parse();

	exit(EXIT_SUCCESS);
}
