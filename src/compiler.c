#include "common/io.h"
#include "lexer/lexer.h"
#include "parser/parser.h"

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

int main(int argc, char **argv) {
	assert(sizeof(char) == 1);
	if(argc != 2) exit(EXIT_FAILURE);
	system("tabs -4");

	lexer_init(argv[1]);
	puts(
		parser_parse() ?
		"\nParsing succeeded!" :
		"\nParsing failed!"
	);

	exit(EXIT_SUCCESS);
}
