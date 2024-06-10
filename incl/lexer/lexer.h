#ifndef LEXER_H
#define LEXER_H

#include "tokens.h"

#include "common/io.h"

extern const char *token_type_strs[];
typedef enum token_type {
	FOREACH_TOKEN(GENERATE_ENUM)
} token_type_t;

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
