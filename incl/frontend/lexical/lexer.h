#ifndef LEXER_H
#define LEXER_H

#include "tokens.h"

#include "common/strslice.h"

extern const char *token_type_strs[];
typedef enum token_type {
	FOREACH_TOKEN(GENERATE_TOK_ENUM)
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
