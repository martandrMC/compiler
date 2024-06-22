#ifndef IO_H
#define IO_H

#include <stdbool.h>
#include <stdio.h>

/** Combination of a preferably null-terminated character array and its
  * length in preferably character count. The string is referred to as
  * "empty" when `string` points to NULL, in which case `size` has no meaning
  * and can be any value. Additionally, `string` may point to a heap allocation,
  * stack allocation or a constant string literal. It is up to context exactly
  * what the meaning of this struct's fields are.
  */
typedef struct string {
	/// Size of the `string` array including the potential null terminator.
	size_t size;
	/// Pointer to string data.
	char *string;
} string_t;

// The canonical configuration of a string_t to indicate that it is empty.
#define EMPTY_STRING (string_t){.size=0, .string=NULL}

/** Attempts to read from the given file in chunks and allocates an array on
  * the heap to exatly fit the contents of it. If allocation is unsuccessful
  * then an empty string is returned instead and errno is set. Use `error_if`
  * to report the error and exit or handle it.
  * @param fdesc The opened file descriptor to read from.
  * @return Returns a string struct with heap-allocated data if successful.
  */
string_t str_read(FILE *fdesc);

/** Prints the last error code with perror and exits with
  * EXIT_FAILURE if the given boolean is true.
  * @param error_condition The condition that you want to check.
  */
void error_if(bool error_condition);

#endif // IO_H
