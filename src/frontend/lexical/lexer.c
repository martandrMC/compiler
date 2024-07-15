#include "lexer.h"

#include "common/arena.h"
#include "common/error.h"
#include "common/strslice.h"

#include <ctype.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "keywords.c"

static struct lexer_state {
	bool reinit;

	string_file_t file;
	size_t file_ptr;

	arena_t list;
	token_list_t *next_ptr;
	token_list_t *last_ptr;
} ls;

// Internal Functions //

static bool is_white_space(char c) {
	switch(c) {
		case ' ' : case '\t':
		case '\r': case '\n':
			return true;
		default:
			return false;
	}
}

static bool is_ident_part(char c) {
	if(c == '_') return true;
	return isalnum(c);
}

static char get_char(bool consume) {
	if(ls.file_ptr >= ls.file.content.size) return '\0';
	char c = ls.file.content.string[ls.file_ptr];
	if(consume) ls.file_ptr++;
	return c;
}

static char skip_until(char c) {
	while(true) {
		char next = get_char(true);
		if(next == c) return get_char(true);
		else if(next == '\0') return next;
	}
}

static void cleanup(void) {
	arena_free(&ls.list);
}

#define RET(x,n) return (token_t) { .type = x,\
.content = CONSTRUCT_STR(n, &ls.file.content.string[ls.file_ptr - n]) }
static token_t read_token(void) {
	char current = get_char(true);

	// Skip whitespaces and comments
	while(true) {
		if(current == '/') {
			char lookahead = get_char(false);
			if(lookahead == '/') current = skip_until('\n');
			else if(lookahead == '*') {
				do current = skip_until('*');
				while(current != '/' && current != '\0');
				current = get_char(true);
			} else break;
		} else if(is_white_space(current)) current = get_char(true);
		else break;
	}

	// Handle symbols and symbol sequences
	switch(current) {
		case '(': RET(TOK_OPEN_ROUND, 1);
		case ')': RET(TOK_CLOSE_ROUND, 1);
		case ',': RET(TOK_COMMA, 1);
		case ':': RET(TOK_COLON, 1);
		case ';': RET(TOK_SEMICOLON, 1);
		case '=': {
			if(get_char(false) == '=') {
				get_char(true);
				RET(TOK_OP_COMPARE, 2);
			} else RET(TOK_OP_ASSIGN, 1);
		}
		case '+': {
			if(get_char(false) == '=') {
				get_char(true);
				RET(TOK_OP_ASSIGN_ALT, 2);
			} else RET(TOK_OP_PLUS, 1);
		}
		case '-': {
			if(get_char(false) == '=') {
				get_char(true);
				RET(TOK_OP_ASSIGN_ALT, 2);
			} else RET(TOK_OP_MINUS, 1);
		}
		case '*': {
			if(get_char(false) == '=') {
				get_char(true);
				RET(TOK_OP_ASSIGN_ALT, 2);
			} else RET(TOK_OP_MULT, 1);
		}
		case '/': {
			if(get_char(false) == '=') {
				get_char(true);
				RET(TOK_OP_ASSIGN_ALT, 2);
			} else RET(TOK_OP_DIV, 1);
		}
		case '%': {
			if(get_char(false) == '=') {
				get_char(true);
				RET(TOK_OP_ASSIGN_ALT, 2);
			} else RET(TOK_OP_MOD, 1);
		}
		case '>': {
			if(get_char(false) == '=') {
				get_char(true);
				RET(TOK_OP_COMPARE, 2);
			} else RET(TOK_OP_COMPARE, 1);
		}
		case '<': {
			char lookahead = get_char(false);
			if(lookahead == '=') {
				get_char(true);
				RET(TOK_OP_COMPARE, 2);
			} else if(lookahead == '>') {
				get_char(true);
				RET(TOK_OP_COMPARE, 2);
			} else RET(TOK_OP_COMPARE, 1);
		}
	}

	if(isdigit(current)) {
		// Handle integer literals
		size_t count = 1;
		while(true) {
			char lookahead = get_char(false);
			if(!is_ident_part(lookahead)) break;
			current = get_char(true);
			count++;
		}
		RET(TOK_LIT_NUM, count);
	} else if(isalpha(current) || current == '_') {
		// Handle identifiers and keywords
		uint8_t hash = 0;
		size_t count = 1;
		while(true) {
			hash = map_sbox[hash ^ current];
			char lookahead = get_char(false);
			if(!is_ident_part(lookahead)) break;
			current = get_char(true);
			count++;
		}
		hash &= MAP_SIZE - 1;
		const char *content = &ls.file.content.string[ls.file_ptr - count];
		const char *keyword = map_keys[hash];
		if(strncmp(content, keyword, count)) RET(TOK_IDENT, count);
		else RET(map_vals[hash], count);
	} else if(current != '\0') {
		string_t error_spot = CONSTRUCT_STR(1, &ls.file.content.string[ls.file_ptr - 1]);
		error_t error_descriptor = err_new_err(ls.file, error_spot, LITERAL_STR("Invalid symbol"));
		err_submit(error_descriptor, false);
		return read_token();
	} else RET(TOK_EOF, 1);
}
#undef RET

static token_list_t *new_allocated_token(void) {
	token_list_t *ret = (token_list_t *) arena_alloc(&ls.list, sizeof(token_list_t));
	ret->token = read_token(), ret->next = NULL;
	return ret;
}

// External Functions //

void lexer_init(string_file_t file) {
	if(ls.reinit) cleanup();
	else atexit(cleanup), ls.reinit = true;

	ls.file = file;
	ls.file_ptr = 0;
	ls.list = arena_new(64 * sizeof(token_list_t));
	ls.last_ptr = new_allocated_token();
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
		ls.last_ptr = new_allocated_token();
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

string_t lexer_get_src(void) {
	return ls.file.content;
}
