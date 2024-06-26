#include "lexer.h"

#include <stdint.h>

// Must be a power of two
#define MAP_SIZE 32

static const uint8_t map_sbox[256] = {
	0xeb, 0xce, 0x84, 0xe9, 0x3b, 0x49, 0x2a, 0x4e, 0x76, 0x9e, 0xaa, 0xf2, 0xfa, 0xa0, 0x74, 0xf0,
	0x21, 0x3c, 0xb9, 0xd3, 0x24, 0x67, 0xfc, 0xae, 0xa2, 0x69, 0x2f, 0xe1, 0xbc, 0x7b, 0xb2, 0x77,
	0xcd, 0x3e, 0x70, 0xf6, 0xde, 0x7f, 0x7c, 0xb5, 0x65, 0x63, 0x85, 0x8b, 0xc6, 0x0d, 0x15, 0xff,
	0xa4, 0x79, 0xa9, 0x38, 0x7a, 0x10, 0x88, 0x61, 0xb1, 0x39, 0xe8, 0xbe, 0x8d, 0xbb, 0x5a, 0xe4,
	0xcc, 0x3f, 0xb7, 0x04, 0xc8, 0x34, 0x44, 0xe5, 0x9b, 0xaf, 0x81, 0x4f, 0xc7, 0xc4, 0xd1, 0xc3,
	0x1a, 0x9a, 0x31, 0x37, 0x56, 0x90, 0x1d, 0x55, 0x53, 0x41, 0x09, 0xba, 0x48, 0x2b, 0x54, 0x26,
	0x9f, 0x82, 0x6b, 0x40, 0x08, 0xbf, 0xfd, 0xd8, 0x95, 0x89, 0xd6, 0xa3, 0xb4, 0x92, 0x42, 0x35,
	0x64, 0xb3, 0x20, 0xe0, 0x73, 0xf5, 0x12, 0x59, 0xdd, 0x96, 0x25, 0x68, 0xc1, 0x03, 0x28, 0x80,
	0x5b, 0x36, 0x19, 0xf4, 0xd5, 0x83, 0x6d, 0x29, 0xac, 0x6e, 0xf9, 0x66, 0x16, 0x7e, 0xc5, 0x99,
	0x0c, 0xf7, 0x62, 0xf1, 0x1f, 0xd4, 0x71, 0x9d, 0xe6, 0x0a, 0x32, 0x3a, 0x2c, 0x0f, 0x45, 0xb6,
	0x33, 0x50, 0x8c, 0xca, 0x8e, 0x87, 0xbd, 0xdf, 0x8a, 0xee, 0x91, 0x43, 0x75, 0x4b, 0xfb, 0x06,
	0xea, 0x02, 0x6c, 0x7d, 0xd9, 0xec, 0xd7, 0x1b, 0x4c, 0xdb, 0x93, 0x3d, 0xc2, 0xa8, 0x9c, 0x23,
	0xc0, 0x00, 0xef, 0x4a, 0x97, 0xc9, 0x18, 0x2e, 0x60, 0x27, 0x52, 0x30, 0x78, 0x5e, 0x11, 0xab,
	0xe3, 0x17, 0x1e, 0x58, 0xfe, 0x22, 0xa7, 0xa6, 0x46, 0xed, 0x5c, 0xa1, 0x1c, 0xd0, 0x98, 0xad,
	0xa5, 0x13, 0x51, 0x6a, 0xf8, 0x94, 0xe2, 0xd2, 0x01, 0x47, 0x05, 0x5d, 0x2d, 0x4d, 0x0e, 0x0b,
	0xe7, 0xdc, 0xb0, 0x8f, 0x6f, 0xda, 0x14, 0xf3, 0xb8, 0x07, 0x72, 0x57, 0xcf, 0xcb, 0x86, 0x5f
};

static const char *map_keys[MAP_SIZE] = {
	"end", "", "var", "", "false", "or", "", "",
	"bool", "", "", "if", "", "", "else", "and",
	"while", "", "nil", "true", "", "", "not", "elif",
	"do", "nat", "", "return", "", "int", "", ""
};

static const token_type_t map_vals[MAP_SIZE] = {
	TOK_KW_END, TOK_IDENT, TOK_KW_VAR, TOK_IDENT,
	TOK_KW_FALSE, TOK_KW_OR, TOK_IDENT, TOK_IDENT,
	TOK_TYPE_BOOL, TOK_IDENT, TOK_IDENT, TOK_KW_IF,
	TOK_IDENT, TOK_IDENT, TOK_KW_ELSE, TOK_KW_AND,
	TOK_KW_WHILE, TOK_IDENT, TOK_KW_NIL, TOK_KW_TRUE,
	TOK_IDENT, TOK_IDENT, TOK_KW_NOT, TOK_KW_ELIF,
	TOK_KW_DO, TOK_TYPE_NAT, TOK_IDENT, TOK_KW_RETURN,
	TOK_IDENT, TOK_TYPE_INT, TOK_IDENT, TOK_IDENT
};
