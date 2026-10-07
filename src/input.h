#ifndef INPUT_H
#define INPUT_H

#include "simulator.h"
#include <stdio.h>

/* Takes ownership of an already-open file and closes it on every path. */
int load_references_stream(FILE *file, ReferenceString *result,
                           char *error, size_t error_size);

/* Transactional: leave result unchanged when loading fails. */
int load_references(const char *path, ReferenceString *result,
                    char *error, size_t error_size);
int save_references(const char *path, const ReferenceString *references,
                    char *error, size_t error_size);
/* 1 = line, 0 = EOF, -1 = line too long (remainder discarded). */
int read_line(char *buffer, size_t capacity);
int parse_integer(const char *text, int minimum, int maximum, int *value);

#endif
