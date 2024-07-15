#include "error.h"

#include "vector.h"

#include <stdio.h>
#include <stdlib.h>

static vector_t *error_queue = NULL;

error_t err_new_err(string_file_t file, string_t spot, string_t message) {
	size_t line = 1;
	char *content = file.content.string;
	char *last_nl = content - 1;

	for(char *c = content; c < spot.string; c++)
		if(*c == '\n') line++, last_nl = c;
	
	return (error_t) {
		.file = file, .row = line,
		.column = spot.string - last_nl,
		.length = spot.size,
		.message = message
	};
}

void err_submit(error_t error, bool fatal) {
	if(error_queue == NULL)
		error_queue = vector_new(NULL, sizeof(error_t), 16);
	vector_add(&error_queue, &error);
	if(fatal) err_print(), exit(EXIT_FAILURE);
}

void err_print(void) {
	if(error_queue == NULL) return;
	for(size_t i=0; i<error_queue->count; i++) {
		error_t *error = vector_peek_from(error_queue, i);
		printf(
			"ERROR: %.*s at row %u, column %u\n",
			(int) error->file.name.size,
			error->file.name.string,
			error->row, error->column
		);
	}
	free(error_queue);
	error_queue = NULL;
}

void error_if(bool error_condition) {
	if(error_condition) {
		perror(NULL);
		exit(EXIT_FAILURE);
	}
}
