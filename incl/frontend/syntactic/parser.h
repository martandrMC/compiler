#ifndef PARSER_H
#define PARSER_H

#include <stdbool.h>

ast_node_t *parser_start(string_file_t file, ast_t *empty);

#endif // PARSER_H
