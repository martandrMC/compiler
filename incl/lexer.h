#ifndef LEXER_H
#define LEXER_H

#include "io.h"

typedef enum {
	// Internal
	TOK_ERROR = 0, TOK_EOF,
	// Structural
	TOK_OPEN_ROUND, TOK_CLOSE_ROUND,
	TOK_OPEN_CURLY, TOK_CLOSE_CURLY,
	TOK_COMMA, TOK_SEMICOLON,
	// Operators
	TOK_ASSIGN, TOK_PLUS, TOK_LESS_THAN,
	// Keywords
	TOK_KW_VAR, TOK_KW_IF, TOK_KW_LOOP,
	TOK_KW_TRUE, TOK_KW_FALSE,
	// Base Types
	TOK_TYPE_INT, TOK_TYPE_BOOL,
	// Others
	TOK_IDENT, TOK_LIT_INT
} tok_type;

typedef struct {
	tok_type type;
	string_t content;
} token_t;

token_t init_lexer(string_t input);
token_t next_token();

#endif // LEXER_H
