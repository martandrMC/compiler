#include "lexer_self.h"
#include "common/io.h"
#include "common/arena.h"

#include <ctype.h>
#include <string.h>

static struct lexer_state {
	string_t input;
	size_t pointer;
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
	if(ls.pointer >= ls.input.size) return '\0';
	char c = ls.input.string[ls.pointer];
	if(consume) ls.pointer++;
	return c;
}

// External Functions //

token_t init_lexer(string_t input) {
	ls.input = input;
	ls.pointer = 0;
	return next_token();
}

#define RET(x,n) return (token_t) { .type = x,\
.content = { .size = n, .string = &ls.input.string[ls.pointer - n] } }
token_t next_token() {
	char current = get_char(true);

	// Skip whitespaces and comments
	for(bool done = false; !done; ) {
		if(current == '/') {
			char lookahead = get_char(false);
			if(lookahead == '/') {
				while(get_char(true) != '\n');
				current = get_char(true);
			} else if(lookahead == '*') {
				get_char(true);
				do while(get_char(true) != '*');
				while(get_char(true) != '/');
				current = get_char(true);
			} else done = true;
		} else if(is_white_space(current))
			current = get_char(true);
		else done = true;
	}

	// Handle symbols and symbol sequences
	switch(current) {
		case '+': RET(TOK_OP_PLUS, 1);
		case '-': RET(TOK_OP_MINUS, 1);
		case '=': RET(TOK_OP_ASSIGN, 1);
		case '<': RET(TOK_OP_LESS_THAN, 1);
		case '>': RET(TOK_OP_GREATER_THAN, 1);
		case '(': RET(TOK_OPEN_ROUND, 1);
		case ')': RET(TOK_CLOSE_ROUND, 1);
		case ',': RET(TOK_COMMA, 1);
		case ':': RET(TOK_COLON, 1);
		case ';': RET(TOK_SEMICOLON, 1);
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
		const char *content = &ls.input.string[ls.pointer - count];
		const char *keyword = map_keys[hash];
		if(strncmp(content, keyword, count)) RET(TOK_IDENT, count);
		else RET(map_vals[hash], count);
	} else if(current == '\0') RET(TOK_EOF, 1);
	else RET(TOK_ERROR, 1);
}
#undef RET
