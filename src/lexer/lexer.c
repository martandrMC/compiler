#include "lexer.h"

#include "common/io.h"
#include "common/arena.h"

#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>

#include "hashmap.c"

static struct lexer_state {
	bool reinit;

	string_t input;
	size_t input_ptr;

	arena_t list;
	token_list_t *next_ptr;
	token_list_t *last_ptr;
} ls;

// Internal Functions //

static bool _is_white_space(char c) {
	switch(c) {
		case ' ' : case '\t':
		case '\r': case '\n':
			return true;
		default:
			return false;
	}
}

static bool _is_ident_part(char c) {
	if(c == '_') return true;
	return isalnum(c);
}

static char _get_char(bool consume) {
	if(ls.input_ptr >= ls.input.size) return '\0';
	char c = ls.input.string[ls.input_ptr];
	if(consume) ls.input_ptr++;
	return c;
}

static void _cleanup_lexer(void) {
	free(ls.input.string);
	arena_free(&ls.list);
}

#define RET(x,n) return (token_t) { .type = x,\
.content = { .size = n, .string = &ls.input.string[ls.input_ptr - n] } }
static token_t _read_token(void) {
	char current = _get_char(true);

	// Skip whitespaces and comments
	for(bool done = false; !done; ) {
		if(current == '/') {
			char lookahead = _get_char(false);
			if(lookahead == '/') {
				while(_get_char(true) != '\n');
				current = _get_char(true);
			} else if(lookahead == '*') {
				do { while(_get_char(true) != '*'); }
				while(_get_char(true) != '/');
				current = _get_char(true);
			} else done = true;
		} else if(_is_white_space(current))
			current = _get_char(true);
		else done = true;
	}

	// Handle symbols and symbol sequences
	switch(current) {
		case '(': RET(TOK_OPEN_ROUND, 1);
		case ')': RET(TOK_CLOSE_ROUND, 1);
		case ',': RET(TOK_COMMA, 1);
		case ':': RET(TOK_COLON, 1);
		case ';': RET(TOK_SEMICOLON, 1);
		case '=': {
			if(_get_char(false) == '=') {
				_get_char(true);
				RET(TOK_OP_EQUAL, 2);
			} else RET(TOK_OP_ASSIGN, 1);
		}
		case '+': {
			if(_get_char(false) == '=') {
				_get_char(true);
				RET(TOK_OP_PLUS_ASSIGN, 2);
			} else RET(TOK_OP_PLUS, 1);
		}
		case '-': {
			if(_get_char(false) == '=') {
				_get_char(true);
				RET(TOK_OP_MINUS_ASSIGN, 2);
			} else RET(TOK_OP_MINUS, 1);
		}
		case '*': {
			if(_get_char(false) == '=') {
				_get_char(true);
				RET(TOK_OP_MULT_ASSIGN, 2);
			} else RET(TOK_OP_MULT, 1);
		}
		case '/': {
			if(_get_char(false) == '=') {
				_get_char(true);
				RET(TOK_OP_DIV_ASSIGN, 2);
			} else RET(TOK_OP_DIV, 1);
		}
		case '%': {
			if(_get_char(false) == '=') {
				_get_char(true);
				RET(TOK_OP_MOD_ASSIGN, 2);
			} else RET(TOK_OP_MOD, 1);
		}
		case '>': {
			if(_get_char(false) == '=') {
				_get_char(true);
				RET(TOK_OP_GREATER_EQ, 2);
			} else RET(TOK_OP_GREATER, 1);
		}
		case '<': {
			char lookahead = _get_char(false);
			if(lookahead == '=') {
				_get_char(true);
				RET(TOK_OP_LESS_EQ, 2);
			} else if(lookahead == '>') {
				_get_char(true);
				RET(TOK_OP_DIFFERENT, 2);
			} else RET(TOK_OP_LESS, 1);
		}
	}

	if(isdigit(current)) {
		// Handle integer literals
		size_t count = 1;
		while(true) {
			char lookahead = _get_char(false);
			if(!_is_ident_part(lookahead)) break;
			current = _get_char(true);
			count++;
		}
		RET(TOK_LIT_NUM, count);
	} else if(isalpha(current) || current == '_') {
		// Handle identifiers and keywords
		uint8_t hash = 0;
		size_t count = 1;
		while(true) {
			hash = map_sbox[hash ^ current];
			char lookahead = _get_char(false);
			if(!_is_ident_part(lookahead)) break;
			current = _get_char(true);
			count++;
		}
		hash &= MAP_SIZE - 1;
		const char *content = &ls.input.string[ls.input_ptr - count];
		const char *keyword = map_keys[hash];
		if(strncmp(content, keyword, count)) RET(TOK_IDENT, count);
		else RET(map_vals[hash], count);
	} else if(current == '\0') RET(TOK_EOF, 1);
	else RET(TOK_ERROR, 1);
}
#undef RET

static token_list_t *_new_allocated_token(void) {
	token_list_t *ret = (token_list_t *) arena_alloc(&ls.list, sizeof(token_list_t));
	ret->token = _read_token(), ret->next = NULL;
	return ret;
}

// External Functions //

void lexer_init(const char *file_path) {
	if(ls.reinit) _cleanup_lexer();
	else atexit(_cleanup_lexer), ls.reinit = true;

	FILE *fdesc = fopen(file_path, "r");
	error_if(!fdesc);
	string_t file = str_read(fdesc);
	error_if(!file.string);
	fclose(fdesc);

	ls.input = file;
	ls.input_ptr = 0;
	ls.list = arena_new(64 * sizeof(token_list_t));
	ls.last_ptr = _new_allocated_token();
	ls.next_ptr = ls.last_ptr;
}

void lexer_backtrack(token_t *next_ptr) {
	uintptr_t struct_addr = (uintptr_t) next_ptr - offsetof(token_list_t, token);
	ls.next_ptr = (token_list_t *) struct_addr;
}

token_t *lexer_next(void) {
	token_list_t *ret;
	if(ls.next_ptr == ls.last_ptr) {
		ret = ls.last_ptr;
		ls.last_ptr = _new_allocated_token();
		ret->next = ls.next_ptr = ls.last_ptr;
	} else {
		ret = ls.next_ptr;
		ls.next_ptr = ls.next_ptr->next;
	}
	return &ret->token;
}

token_t *lexer_peek(void) {
	return &ls.last_ptr->token;
}
