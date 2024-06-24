#include "vector.h"

vector_t *vector_new(arena_t *arena, size_t unit_size, size_t capacity) {
	size_t initial_size_bytes = sizeof(vector_t) + unit_size * capacity;
	vector_t *vector = (vector_t *) arena_alloc(arena, initial_size_bytes);
	vector->unit_size = unit_size;
	vector->capacity = capacity;
	vector->count = 0;
	return vector;
}

void vector_add(vector_t **vector, const void *data) {
	// TODO
}

void vector_take(vector_t **vector, void *data) {
	// TODO
}

void *vector_get(const vector_t *vector) {
	// TODO
	return NULL;
}

void vector_add_to(vector_t **vector, const void *data, size_t index) {
	// TODO
}

void vector_take_from(vector_t **vector, void *data, size_t index) {
	// TODO
}

void *vector_get_from(const vector_t *vector, size_t index) {
	// TODO
	return NULL;
}
