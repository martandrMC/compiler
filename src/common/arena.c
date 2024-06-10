#include "arena.h"
#include "io.h"

#include <assert.h>
#include <stdlib.h>

// Internal Functions //

static region_t *_create_region(size_t block_size) {
	// round up to the next highest mutliple of uintptr_t
	size_t header = (sizeof(region_t) - 1) / sizeof(uintptr_t) + 1;
	// ask libc for a new zeroed region and initialise it
	region_t *region = (region_t *) calloc(header + block_size, sizeof(uintptr_t));
	error_if(region == NULL);
	region->next = NULL, region->used = 0; // redundant
	region->size = block_size;
	return region;
}

static void *_alloc_first(arena_t *arena, size_t block_size_bytes) {
	// this function should only have been called on empty arenas
	assert(arena->first == NULL && arena->last == NULL);
	
	// calculate the rounded up size of the block and use that for
	// the size of the new region if it's bigger than the minimum
	size_t block_size = (block_size_bytes - 1) / sizeof(uintptr_t) + 1;
	size_t new_region_size = arena->min_region_size;
	if(new_region_size < block_size) new_region_size = block_size;

	// create the region and link it to the arena
	region_t *new_region = _create_region(new_region_size);
	if(new_region == NULL) return NULL;
	arena->first = arena->last = new_region;

	// no need index into data, it's guaranteed to be empty
	void *block = (void *) &arena->last->data;
	arena->last->used += block_size;
	return block;
}

// External Functions //

arena_t arena_new(size_t min_region_size_bytes) {
	// round up to the next highest mutliple of uintptr_t
	size_t min_region_size = (min_region_size_bytes - 1) / sizeof(uintptr_t) + 1;
	return (arena_t) {
		.min_region_size = min_region_size,
		.first = NULL, .last = NULL
	};
}

void *arena_alloc(arena_t *arena, size_t block_size_bytes) {
	// call the first allocation function instead if the arena is empty
	if(arena->first == NULL) {
		assert(arena->last == NULL);
		return _alloc_first(arena, block_size_bytes);
	}

	// * the block size calculation can be pulled out the loop
	size_t block_size = (block_size_bytes - 1) / sizeof(uintptr_t) + 1;
	for(region_t *curr = arena->last; curr != NULL; curr = curr->next) {
		// if it fits into the current region then we are done
		if(block_size + curr->used <= curr->size) break;
		// if instead we reached the last region and it still doesn't fit
		if(curr->next == NULL) {
			// calculate the rounded up size of the block and use that for
			// the size of the new region if it's bigger than the minimum *
			size_t new_region_size = arena->min_region_size;
			if(new_region_size < block_size) new_region_size = block_size;

			// create the region and link it to the previously last one
			region_t *new_region = _create_region(new_region_size);
			if(new_region == NULL) return NULL;
			curr->next = new_region;
		}
		arena->last = curr->next;
	}

	void *block = (void *) &arena->last->data[arena->last->used];
	arena->last->used += block_size;
	return block;
}

void arena_free(arena_t *arena) {
	for(
		// iterate over all regions *
		region_t *curr = arena->first;
		curr != NULL;
	) {
		// we can't free before we follow next
		region_t *tmp = curr;
		curr = curr->next; // * increment here
		free(tmp);
	}
	arena->first = NULL;
	arena->last = NULL;
}
