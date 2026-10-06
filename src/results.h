#ifndef RESULTS_H
#define RESULTS_H

#include "simulator.h"

#define RESULT_PATH_CAPACITY 4096
#define RESULT_NAME_CAPACITY 256
#define MAX_RESULT_FILES 256

typedef struct {
    char name[RESULT_NAME_CAPACITY];
    size_t references;
} ResultFile;

typedef struct {
    ResultFile files[MAX_RESULT_FILES];
    size_t count;
    size_t invalid;
    size_t omitted;
} ResultCatalog;

/* On Windows the folder is next to build/, independent of the shell's cwd. */
int initialize_results(char *directory, size_t capacity, char *error, size_t error_size);
int ensure_results_directory(const char *directory, char *error, size_t error_size);
/* Bare names resolve inside results/; paths containing a slash or colon are preserved. */
int resolve_reference_path(const char *entry, const char *directory,
                           char *path, size_t capacity, char *error, size_t error_size);
int next_result_path(const char *directory, char *path, size_t capacity,
                     char *error, size_t error_size);
/* List only regular .txt files accepted by the reference-string parser. */
int list_result_files(const char *directory, ResultCatalog *catalog,
                      char *error, size_t error_size);

#endif
