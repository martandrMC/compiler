#ifndef LEXER_H
#define LEXER_H

#include "common/io.h"

typedef enum {
	// Internal
	TOK_ERROR = 0, TOK_EOF,
	// Structural
	TOK_OPEN_ROUND, TOK_CLOSE_ROUND,
	TOK_COMMA, TOK_COLON, TOK_SEMICOLON,
	// Operators
	TOK_OP_ASSIGN, TOK_OP_PLUS, TOK_OP_MINUS,
	TOK_OP_LESS_THAN, TOK_OP_GREATER_THAN,
	// Keywords
	TOK_KW_DO, TOK_KW_END, TOK_KW_RETURN,
	TOK_KW_VAR, TOK_KW_IF, TOK_KW_WHILE,
	TOK_KW_TRUE, TOK_KW_FALSE, TOK_KW_NIL,
	TOK_KW_AND, TOK_KW_OR, TOK_KW_NOT,
	// Base Types
	TOK_TYPE_NAT, TOK_TYPE_INT, TOK_TYPE_BOOL,
	// Others
	TOK_IDENT, TOK_LIT_NUM
} tok_type;

typedef struct token {
	tok_type type;
	string_t content;
} token_t;

token_t init_lexer(string_t input);
token_t next_token();

#endif // LEXER_H
