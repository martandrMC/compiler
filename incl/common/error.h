#ifndef ERROR_H
#define ERROR_H

#include "strslice.h"

#include <stdbool.h>

/** A struct to represent a position in a text file.
  * Both `column` and `row` are one-indexed so that they can be
  * directly printed. This struct is primarally intended for
  * storing the pre-computed location of errors to be displayed.
  */
typedef struct location {
	/// The name of the file that is being referenced.
	string_t file;
	/// The inset from the start of the line.
	size_t column;
	/// The line number.
	size_t row;
} location_t;

/** TODO: Documentation here
  * 
  */
location_t calculate_location(string_file_t file, string_t spot);

/** Prints the last error code with perror and exits with
  * EXIT_FAILURE if the given boolean is true.
  * @param error_condition The condition that you want to check.
  */
void error_if(bool error_condition);

#endif // ERROR_H
