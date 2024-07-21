#include "error.h"

#include "common/vector.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

static struct error_state {
	bool init;
	arena_t arena;
	vector_t *vector;
} es = {.init = false};

// Internal Functions //

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

static void cleanup(void) {
	assert(es.init);
	arena_free(&es.arena);
	es.init = false;
}

// External Functions //

void err_init(void) {
	if(es.init) cleanup();
	es.arena = arena_new(1024);
	es.vector = vector_new(&es.arena, sizeof(error_t), 16);
	es.init = true;
}

arena_t *err_get_arena(void) {
	if(!es.init) return NULL;
	else return &es.arena;
}

error_t err_new(string_file_t file, string_t spot, string_t message) {
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
	assert(es.init);
	vector_add(&es.vector, &error);
	if(fatal) err_finalize(), exit(EXIT_FAILURE);
}

void err_finalize(void) {
	for(size_t i = 0; i < es.vector->count; i++) {
		error_t *error = vector_peek_from(es.vector, i);
		printf(
			"\x1b[1;31mERROR:\x1b[37m %.*s at line %u, column %u\x1b[0m\n",
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
			printf(
				" \x1b[1;36m%.*d |\x1b[0m %.*s\n",
				(int) digits, lnum + 1,
				(int) line.size, line.string
			);
			if(lnum == error->row) {
				for(unsigned i=0; i<digits+2; i++) putchar(' ');
				printf("\x1b[1;36m|\x1b[0m");
				for(unsigned i=0; i<error->column+1; i++) putchar(' ');
				printf("\x1b[1;35m^");
				for(size_t i=0; i<error->length-1; i++) putchar('~');
				printf(
					" %.*s\x1b[0m\n",
					(int) error->message.size,
					error->message.string
				);
			}
		}
	}
	cleanup();
}

void error_if(bool error_condition) {
	if(error_condition) {
		perror(NULL);
		exit(EXIT_FAILURE);
	}
}
