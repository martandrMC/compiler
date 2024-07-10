#include "common/io.h"
#include "frontend/lexical/lexer.h"
#include "frontend/syntactic/parser.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv) {
	// I will keep this here for my future self to laugh at.
	// The C99 standard guarantees chars to be of size 1.
	//assert(sizeof(char) == 1);
	if(argc != 2) exit(EXIT_FAILURE);

	lexer_init(argv[1]);
	parser_start();

	exit(EXIT_SUCCESS);
}
