#ifndef ERROR_H
#define ERROR_H

#include <stdbool.h>

/** Prints the last error code with perror and exits with
  * EXIT_FAILURE if the given boolean is true.
  * @param error_condition The condition that you want to check.
  */
void error_if(bool error_condition);

#endif // ERROR_H
