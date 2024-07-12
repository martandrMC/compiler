#include "error.h"

#include <stdio.h>
#include <stdlib.h>

void error_if(bool error_condition) {
	if(error_condition) {
		perror(NULL);
		exit(EXIT_FAILURE);
	}
}
