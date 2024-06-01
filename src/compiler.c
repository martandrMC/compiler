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
	parser_start();

	exit(EXIT_SUCCESS);
}
