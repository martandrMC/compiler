#include "vector.h"

#include <string.h>

vector_t *vector_new(arena_t *arena, size_t unit_size, size_t capacity) {
	size_t initial_size_bytes = sizeof(vector_t) + unit_size * capacity;
	vector_t *vector;
	if(arena == NULL) {
		vector = (vector_t *) malloc(initial_size_bytes);
		memset(vector, 0, initial_size_bytes);
	} else vector = (vector_t *) arena_alloc(arena, initial_size_bytes);
	vector->unit_size = unit_size;
	vector->capacity = capacity;
	vector->arena = arena;
	vector->count = 0;
	return vector;
}

void vector_add(vector_t **vector, const void *data) {
	vector_t *my_vec = *vector;
	size_t data_size_bytes = my_vec->unit_size * my_vec->capacity;
	if(my_vec->count == my_vec->capacity) {
		size_t new_size_bytes = sizeof(vector_t) + data_size_bytes * 2;
		vector_t * new_vec = NULL;
		if(my_vec->arena != NULL) {
			new_vec = arena_alloc(my_vec->arena, new_size_bytes);
			memcpy(new_vec, my_vec, sizeof(vector_t) + data_size_bytes);
		} else new_vec = realloc(my_vec,  new_size_bytes);
		my_vec = new_vec;
	}

	size_t data_index = my_vec->count * my_vec->unit_size;
	memcpy(&my_vec->data[data_index], data, my_vec->unit_size);
	*vector = my_vec;
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
