#ifndef VECTOR_H
#define VECTOR_H

#include "arena.h"

#include <stdint.h>
#include <stdlib.h>

typedef struct vector {
	arena_t *arena;
	size_t count;
	size_t capacity;
	size_t unit_size;
	uint8_t data[];
} vector_t;

vector_t *vector_new(arena_t *arena, size_t unit_size, size_t capacity);

void vector_add(vector_t **vector, const void *data);
void vector_take(vector_t *vector, void *data);
void *vector_get(const vector_t *vector);

void vector_add_to(vector_t **vector, const void *data, size_t index);
void vector_take_from(vector_t *vector, void *data, size_t index);
void *vector_get_from(const vector_t *vector, size_t index);

#endif // VECTOR_H
