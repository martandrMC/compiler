#include "io.h"

#include <stdlib.h>
#include <string.h>

string_t str_read(FILE *fdesc) {
	const size_t buffer_size =  4096;
	char read_buffer[buffer_size];
	string_t result = { .size = 0, .string = NULL };

	for(bool finished = false; !finished; ) {
		size_t read_amount = fread(read_buffer, 1, buffer_size, fdesc);
		if(read_amount < buffer_size) finished = true, read_amount++;

		size_t new_size = result.size + read_amount;
		char *new_string = (char *) realloc(result.string, new_size);
		if(!new_string) {
			if(result.string) free(result.string);
			result.string = NULL;
			return result;
		}

		memcpy(&new_string[result.size], read_buffer, read_amount);
		result.size += read_amount, result.string = new_string;
		if(finished) result.string[result.size - 1] = '\0';
	}
	
	return result;
}

void str_write(FILE *fdesc, string_t str) {
	for(size_t i=0; i<str.size; i++) {
		char c = str.string[i];
		if(c == '\0') break;
		fputc(c, fdesc);
	}
}

void error_if(bool error_condition) {
	if(error_condition) {
		perror(NULL);
		exit(EXIT_FAILURE);
	}
}
