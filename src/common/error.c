#include "error.h"

#include <stdio.h>
#include <stdlib.h>

location_t calculate_location(string_file_t file, string_t spot) {
	size_t line = 1;
	char *content = file.content.string;
	char *last_nl = content - 1;

	for(char *c = content; c < spot.string; c++)
		if(*c == '\n') line++, last_nl = c;
	
	return (location_t) {
		.file = file.name,
		.column = spot.string - last_nl,
		.row = line
	};
}

void error_if(bool error_condition) {
	if(error_condition) {
		perror(NULL);
		exit(EXIT_FAILURE);
	}
}
