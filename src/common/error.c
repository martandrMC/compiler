#include "error.h"

#include "vector.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

static vector_t *error_queue = NULL;

static unsigned digit_count(unsigned num) {
	unsigned ret = 1;
	for(; num >= 10; num /= 10) ret++;
	return ret;
}

static string_t get_line(string_t full, unsigned line) {
	string_t ret = EMPTY_STRING;
	ret.string = full.string - 1;
	size_t i = 0;
	for(; i<full.size && line > 0; i++) {
		char *ptr = &full.string[i];
		if(*ptr == '\n') line--, ret.string = ptr;
	}
	if(line != 0) return EMPTY_STRING;
	ret.string++;
	for(; i<full.size; i++) {
		char *ptr = &full.string[i];
		if(*ptr == '\n') break;
		ret.size++;
	}
	return ret;
}

error_t err_new_err(string_file_t file, string_t spot, string_t message) {
	size_t line = 0;
	char *content = file.content.string;
	char *last_nl = content - 1;

	for(char *c = content; c < spot.string; c++)
		if(*c == '\n') line++, last_nl = c;

	return (error_t) {
		.file = file, .row = line,
		.column = spot.string - last_nl - 1,
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
			error->row + 1, error->column + 1
		);

		unsigned digits = (int) digit_count(error->row + LINE_SPAN - 1);
		unsigned min_line = error->row < LINE_SPAN ? 0 : error->row - LINE_SPAN;
		unsigned max_line = error->row + LINE_SPAN;
		if(max_line >= error->file.lines) max_line = error->file.lines - 1;
		for(unsigned lnum = min_line; lnum <= max_line; lnum++) {
			string_t line = get_line(error->file.content, lnum);
			printf(" %.*d | %.*s\n", (int) digits, lnum + 1, (int) line.size, line.string);
			if(lnum == error->row) {
				for(unsigned i=0; i<digits+2; i++) putchar(' ');
				putchar('|');
				for(unsigned i=0; i<error->column+1; i++) putchar(' ');
				putchar('^');
				for(size_t i=0; i<error->length-1; i++) putchar('~');
				putchar(' ');
				printf("%.*s", (int) error->message.size, error->message.string);
				putchar('\n');
			}
		}
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
