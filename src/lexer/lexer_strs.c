#include "lexer.h"

#define GENERATE_STRS(VAL) #VAL,
const char *token_type_strs[] = {
	FOREACH_TOKEN(GENERATE_STRS)
};
#undef GENERATE_STRS
