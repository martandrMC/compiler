#ifndef STRSLICE_H
#define STRSLICE_H

#include <stdio.h>

/** A general-usage string slice. The string is said to be empty when `string`
  * is NULL or when `size` is 0. It is best practice to ensure both of those
  * conditions are met though there is no strict way this struct ought to be
  * used. The string nominally ends either when `size` characters have been
  * iterated over or a null byte was found. `string` may or may not be owned
  * and it may be heap allocated, stack allocated, or a read-only literal. There
  * is no guarantee of the exact properties of `string` and how it needs to
  * be handled.
  */
typedef struct string {
	/// Size of the `string` array including the potential null terminator.
	size_t size;
	/// Pointer to string data or NULL when the string is empty.
	char *string;
} string_t;

// The canonical configuration of a `string_t` to indicate that it is empty.
#define EMPTY_STRING ((string_t){.size=0, .string=NULL})

// Convenience macro to encapsulate a C-style string literal into a `string_t`.
#define TO_STRING(literal) ((string_t){.size=(sizeof(literal)-1), .string=literal})

/** A struct to represent a file that has been read into memory.
  * TODO: Better description here.
  */
typedef struct string_file {
	/// Absolute or relative path to the file whose contents are in `content`.
	string_t name;
	/// The contents of the file that where read into memory or `EMPTY_STRING`
	/// if the file is yet to be read. Nominally heap-allocated.
	string_t content;
} string_file_t;

/** Attempts to read from the given file in chunks and allocates an array on
  * the heap to exatly fit the contents of it. If allocation is unsuccessful
  * then an empty string is returned instead and errno is set. Use `error_if`
  * to report the error and exit or handle it.
  * @param fdesc The opened file descriptor to read from.
  * @return Returns a string struct with heap-allocated data if successful.
  */
string_t str_read(FILE *fdesc);

#endif // STRSLICE_H
