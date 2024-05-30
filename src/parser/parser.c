#include "parser.h"
#include "lexer/lexer.h"

#include <stdio.h>
#include <string.h>
#include <stdarg.h>

typedef enum se_variant {
	STMT_EXPR,
	STMT_EXPR_SEM,
	STMT_EXPR_END
} se_variant_t;

typedef enum log_action {
	ACTION_PRINT,
	ACTION_ENTER,
	ACTION_EXIT
} log_action_t;

// Internal Functions (Helpers) //

static token_type_t _parser_peek(void) {
	token_t *ret = lexer_next();
	lexer_backtrack(ret);
	return ret->type;
}

static bool _parser_consume(token_type_t type) {
	return lexer_next()->type == type;
}

static bool _placeholder(void) {
	token_t *tok = lexer_next();
	return 
		tok->type == TOK_IDENT &&
		tok->content.size == 1 &&
		tok->content.string[0] == '_'
	;
}

static void _parser_log(log_action_t action, ...) {
	static unsigned nesting = 0;
	va_list args;
	switch(action) {
		case ACTION_ENTER:
			nesting++;
			break;
		case ACTION_EXIT:
			if(nesting > 0)
			nesting--;
			break;
		case ACTION_PRINT:
			va_start(args, action);
			const char *fmt = va_arg(args, char *);
			for(unsigned i=0; i<nesting; i++) 
				printf("|\t");
			printf("|- ");
			vprintf(fmt, args);
			va_end(args);
			break;
	}
}

// Internal Functions (Non-Terminals) //

static bool _nt_type(void) {
	switch(lexer_next()->type) {
		case TOK_TYPE_NAT:
		case TOK_TYPE_INT:
		case TOK_TYPE_BOOL:
			return true;
		default:
			return false;
	}
}

static bool _nt_block(void) {
	_parser_log(ACTION_PRINT, "BLOCK\n");
	_parser_log(ACTION_ENTER);
	bool success = true;
	for(bool done = false; !done; ) switch(_parser_peek()) {
		case TOK_KW_END: case TOK_EOF: done = true; break;
		case TOK_KW_VAR:
			_parser_log(ACTION_PRINT, "VARDECL\n");
			success &= _parser_consume(TOK_KW_VAR);
			break;
		default:
			done = true;
			success = false;
			break;
	}
	_parser_log(ACTION_EXIT);
	return success;
}

// External Functions //

bool parser_parse(void) {
	bool success = true;
	success &= _nt_block();
	success &= _parser_consume(TOK_EOF);
	return success;
}
