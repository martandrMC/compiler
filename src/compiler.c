#include "common/strslice.h"
#include "frontend/error.h"
#include "frontend/lexical/lexer.h"
#include "frontend/syntactic/parser.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char **argv) {
	// I will keep this here for my future self to laugh at.
	// The C99 standard guarantees chars to be of size 1.
	// assert(sizeof(char) == 1);
	if(argc != 2) exit(EXIT_FAILURE);

	FILE *fdesc = fopen(argv[1], "r");
	error_if(!fdesc);
	string_file_t file;
	file.name.string = argv[1];
	file.name.size = strlen(argv[1]);
	file.content = str_read(fdesc);
	file.lines = str_count_lines(file.content);
	error_if(!file.content.string);
	fclose(fdesc);

	err_init();
	lexer_init(file);
	parser_start(file);
	err_finalize();

	free(file.content.string);
	exit(EXIT_SUCCESS);
}
