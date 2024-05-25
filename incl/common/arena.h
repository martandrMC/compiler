#ifndef _ARENA_H_
#define _ARENA_H_

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

/** Dynamically sized struct encompassing a block of allocatable space by the
  * arena. The arena itself is a collection of these regions arranged into a
  * singly-linked list.
  */
typedef struct region {
	/// Pointer to the next region in the linked list or `NULL` if last.
	struct region *next;
	/// Count of how many elements are allocated from the `data` array.
	size_t used;
	/// Count of homw many elements can maximally fit in the `data` array.
	size_t size;
	/// A dynamic array of elements representing the allocatable space.
	uintptr_t data[];
} region_t;

/** Struct representing an arena. All functions that opererate on arenas
  * expect a pointer to one of these structs. Create one manually if you
  * know what you're doing or use `arena_new` to get a new empty one.
  */
typedef struct arena {
	/// Allocations that create a new region use this value as its minimum size.
	size_t min_region_size;
	/// Pointer to the first region in the linked list of regions.
	region_t *first;
	/// Pointer to the last region that contains allocations.
	region_t *last;
} arena_t;

/** Creates a new empty `arena_t` struct. No regions are allocated initially.
  * An arena is a supplemental allocation scheme ontop of `malloc` that allows
  * allocations with a similar purpose to be grouped together and deallocated
  * all at once when their need expires. This implementation specifically uses
  * the very simple bump allocator which does not allow freeing of elements at
  * the middle of an allocated space.
  * @param min_region_size_bytes The minimum size that regions of this arena
  * will be allocated to be. Internally rounded to the next higher multiple of
  * the size of `uintptr_t`. Set this to a lower value if you expect big and
  * small element allocations intermixed and to a higher value if you expect a
  * more uniform allocation size.
  * @return The newly created `arena_t` struct.
  */
arena_t arena_new(size_t min_region_size_bytes);

/** Attempts to allocate a block of memory to an arena using a constant time
  * complexity bump allocator. The `last` field in the `arena` struct is used
  * as the starting point for the lookup of viable allocation space. If the
  * last region is reached but no space has been found a new region is created.
  * @param arena The arena to allocate the block into.
  * @param block_size_bytes The amount of bytes the new block will be.
  * Rounded up to a multiple of the size of `uintptr_t`.
  * @return The address of the newly allocated block or `NULL` if during the
  * process there was a need to allocate a new region and `malloc` returned
  * `NULL`.
  */
void *arena_alloc(arena_t *arena, size_t block_size_bytes);

/** Clears the arena of all allocations and removes and `free`s all of its
  * regions. The arena is ultimately left to a state equivalent to if it was
  * just created with `arena_new`.
  * @param arena The arena to clear all allocations from.
  */
void arena_free(arena_t *arena);

/** Clears the arena of all allocations but does not remove its regions.
  * @param arena The arena to clear all allocations from.
  */
void arena_empty(arena_t *arena);

#endif // _ARENA_H_
