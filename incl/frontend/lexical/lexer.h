#ifndef LEXER_H
#define LEXER_H

#include "common/strslice.h"

extern const char *token_type_strs[];
typedef enum token_type {
	TOK_ERROR = 0, TOK_EOF,
	TOK_OPEN_ROUND, TOK_CLOSE_ROUND,
	TOK_COMMA, TOK_COLON, TOK_SEMICOLON,
	TOK_OP_ASSIGN, TOK_OP_ASSIGN_ALT,
	TOK_OP_COMPARE, TOK_OP_PLUS, TOK_OP_MINUS,
	TOK_OP_MULT, TOK_OP_DIV, TOK_OP_MOD,
	TOK_KW_DO, TOK_KW_END, TOK_KW_VAR, TOK_KW_RETURN,
	TOK_KW_IF, TOK_KW_ELIF, TOK_KW_ELSE, TOK_KW_WHILE,
	TOK_KW_AND, TOK_KW_OR, TOK_KW_NOT, 
	TOK_KW_TRUE, TOK_KW_FALSE, TOK_KW_NIL,
	TOK_TYPE_NAT, TOK_TYPE_INT, TOK_TYPE_BOOL,
	TOK_IDENT, TOK_LIT_NUM
} token_type_t;

typedef struct token {
	token_type_t type;
	string_t content;
} token_t;

typedef struct token_list {
	struct token_list *next;
	token_t token;
} token_list_t;

void lexer_init(string_file_t file);
void lexer_backtrack(token_t *next_ptr);
token_t *lexer_next(void);
token_t *lexer_peek(void);
string_t lexer_get_src(void);

#endif // LEXER_H
