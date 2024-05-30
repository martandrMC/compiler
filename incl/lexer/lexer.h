#ifndef LEXER_H
#define LEXER_H

#include "common/io.h"

#define FOREACH_TOKEN(FN) \
	/* Internal */ \
	FN(ERROR) FN(EOF) \
	/* Structural */ \
	FN(OPEN_ROUND) FN(CLOSE_ROUND) \
	FN(COMMA) FN(COLON) FN(SEMICOLON) \
	/* Operators */ \
	FN(OP_ASSIGN) FN(OP_PLUS_ASSIGN) FN(OP_MINUS_ASSIGN)\
	FN(OP_MULT_ASSIGN) FN(OP_DIV_ASSIGN) FN(OP_MOD_ASSIGN) \
	FN(OP_PLUS) FN(OP_MINUS) FN(OP_MULT) FN(OP_DIV) FN(OP_MOD) \
	FN(OP_EQUAL) FN(OP_DIFFERENT) FN(OP_GREATER) FN(OP_LESS) \
	FN(OP_GREATER_EQ) FN(OP_LESS_EQ) \
	/* Keywords */ \
	FN(KW_DO) FN(KW_END) FN(KW_VAR) FN(KW_RETURN) \
	FN(KW_IF) FN(KW_ELSE) FN(KW_WHILE) \
	FN(KW_AND) FN(KW_OR) FN(KW_NOT) \
	FN(KW_TRUE) FN(KW_FALSE) FN(KW_NIL) \
	/* Base Types */ \
	FN(TYPE_NAT) FN(TYPE_INT) FN(TYPE_BOOL) \
	/* Others */ \
	FN(IDENT) FN(LIT_NUM)

#define GENERATE_ENUM(VAL) TOK_##VAL,
extern const char *token_type_strs[];
typedef enum token_type {
	FOREACH_TOKEN(GENERATE_ENUM)
} token_type_t;
#undef GENERATE_ENUM

typedef struct token {
	token_type_t type;
	string_t content;
} token_t;

typedef struct token_list {
	struct token_list *next;
	token_t token;
} token_list_t;

void lexer_init(const char *file_path);
void lexer_backtrack(token_t *next_ptr);
token_t *lexer_next(void);
token_t *lexer_peek(void);

#endif // LEXER_H
