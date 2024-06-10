#include "lexer.h"

#include <stdint.h>

// Must be a power of two
#define MAP_SIZE 16

static const uint8_t map_sbox[256] = {
	0xbe, 0xb6, 0x91, 0xa3, 0x8a, 0xd5, 0xdf, 0xfe, 0xb5, 0x00, 0x3f, 0x0e, 0x1b, 0xff, 0xda, 0x20,
	0x19, 0x3d, 0x05, 0x85, 0x28, 0xe8, 0x70, 0x26, 0x2a, 0x87, 0xdd, 0xf8, 0x3b, 0x8c, 0x45, 0x96,
	0x4a, 0x04, 0xe6, 0xd2, 0x33, 0x07, 0x15, 0xba, 0x3a, 0xde, 0x37, 0xc4, 0x39, 0x90, 0x63, 0x73,
	0x08, 0xc7, 0x92, 0xd6, 0xe3, 0x4b, 0x40, 0x44, 0xf2, 0xf0, 0x86, 0x8f, 0x35, 0x52, 0xa9, 0x5e,
	0xa5, 0xa8, 0xec, 0xdc, 0x99, 0x3c, 0x79, 0xcf, 0xc3, 0x21, 0x09, 0xb3, 0x5f, 0x5a, 0x29, 0xb2,
	0x94, 0xdb, 0x7f, 0xa2, 0xad, 0xf1, 0x46, 0x01, 0x03, 0x9a, 0x74, 0xb0, 0xf3, 0x65, 0xb4, 0xf5,
	0x50, 0xa7, 0x6a, 0x78, 0x1a, 0xfc, 0x5b, 0x9f, 0x1d, 0x12, 0xa0, 0xab, 0xc2, 0xd8, 0x7c, 0xea,
	0xfb, 0xb9, 0xe5, 0xd7, 0xbd, 0xef, 0xd4, 0x2e, 0x6e, 0x53, 0x8b, 0xfa, 0x54, 0x97, 0x56, 0x4e,
	0x24, 0x57, 0xcd, 0x67, 0x72, 0x0d, 0x11, 0xb8, 0x34, 0xf6, 0xc0, 0xce, 0x1c, 0x61, 0x9e, 0xe2,
	0xfd, 0x64, 0xe9, 0xe4, 0xcc, 0xca, 0xd1, 0x14, 0xd3, 0x1f, 0x55, 0x38, 0x2b, 0x58, 0xe0, 0x8d,
	0x4d, 0x30, 0x2c, 0x95, 0xa6, 0x16, 0xaa, 0xcb, 0x83, 0xbb, 0x02, 0xc9, 0xf9, 0x49, 0x66, 0xb7,
	0x9c, 0x47, 0x88, 0x7d, 0x06, 0x0f, 0x4f, 0xc5, 0x3e, 0xaf, 0x6b, 0x71, 0x69, 0x41, 0x81, 0x6d,
	0xd9, 0x0a, 0xe7, 0x4c, 0xf4, 0x48, 0x42, 0xc6, 0x80, 0x68, 0xe1, 0xc1, 0x22, 0xb1, 0x6f, 0x0b,
	0x10, 0x7e, 0x1e, 0x82, 0x7b, 0x36, 0xae, 0xbf, 0x59, 0x31, 0x17, 0xeb, 0x51, 0xd0, 0xac, 0x6c,
	0x76, 0x2d, 0xa1, 0x98, 0xa4, 0x2f, 0x62, 0x13, 0x8e, 0x0c, 0x9d, 0xed, 0x93, 0x60, 0x75, 0xc8,
	0x43, 0x84, 0x5c, 0xee, 0x18, 0x9b, 0x5d, 0x7a, 0x25, 0xf7, 0x32, 0xbc, 0x77, 0x89, 0x27, 0x23
};

static const char *map_keys[MAP_SIZE] = {
	"while", "end", "nil", "or", "not", "nat", "true", "var",
	"return", "false", "int", "and", "else", "if", "bool", "do"
};

static const token_type_t map_vals[MAP_SIZE] = {
	TOK_KW_WHILE, TOK_KW_END, TOK_KW_NIL, TOK_KW_OR,
	TOK_KW_NOT, TOK_TYPE_NAT, TOK_KW_TRUE, TOK_KW_VAR,
	TOK_KW_RETURN, TOK_KW_FALSE, TOK_TYPE_INT, TOK_KW_AND,
	TOK_KW_ELSE, TOK_KW_IF, TOK_TYPE_BOOL, TOK_KW_DO
};
