#ifndef ERROR_H
#define ERROR_H

#include "common/arena.h"
#include "common/strslice.h"

#include <stdbool.h>

#define LINE_SPAN 1

/** A struct to represent a reported error and all info neded to
  * appropriately present it to the user.
  */
typedef struct error {
	/// The file that is being referenced.
	string_file_t file;
	/// The line number.
	unsigned row;
	/// The offset from the start of the line.
	unsigned column;
	/// The amount of characters in the span.
	size_t length;
	/// The message detailing the nature of the error.
	string_t message;
} error_t;

void err_init(void);
arena_t *err_get_arena(void);

error_t err_new(string_file_t file, string_t spot, string_t message);
void err_submit(error_t error, bool fatal);
void err_finalize(void);

/** Prints the last error code with perror and exits with
  * EXIT_FAILURE if the given boolean is true.
  * @param error_condition The condition that you want to check.
  */
void error_if(bool error_condition);

#endif // ERROR_H
