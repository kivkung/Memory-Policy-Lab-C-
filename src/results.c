#include "results.h"
#include "input.h"

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#ifdef _WIN32
#include <direct.h>
#include <io.h>
#include <windows.h>
#define PATH_SEPARATOR "\\"
#else
#include <dirent.h>
#include <unistd.h>
#define PATH_SEPARATOR "/"
#endif

static int compare_names(const char *left, const char *right)
{
    const unsigned char *p = (const unsigned char *)left;
    const unsigned char *q = (const unsigned char *)right;
    while (*p && *q) {
        int difference = tolower(*p) - tolower(*q);
        if (difference) return difference;
        ++p;
        ++q;
    }
    return (int)*p - (int)*q;
}

static int join_path(const char *directory, const char *name,
                     char *path, size_t capacity, char *error, size_t error_size)
{
    int length = snprintf(path, capacity, "%s%s%s", directory, PATH_SEPARATOR, name);
    if (length < 0 || (size_t)length >= capacity) {
        snprintf(error, error_size, "The file path is too long.");
        return 0;
    }
    return 1;
}

static int text_extension(const char *name)
{
    size_t n = strlen(name);
    return n >= 4 && name[n - 4] == '.' &&
        tolower((unsigned char)name[n - 3]) == 't' &&
        tolower((unsigned char)name[n - 2]) == 'x' &&
        tolower((unsigned char)name[n - 1]) == 't';
}

int ensure_results_directory(const char *directory, char *error, size_t error_size)
{
    struct stat info;
    if (stat(directory, &info) == 0) {
#ifdef _WIN32
        if ((info.st_mode & S_IFMT) == S_IFDIR) return 1;
#else
        if (S_ISDIR(info.st_mode)) return 1;
#endif
        snprintf(error, error_size, "The results path exists but is not a directory.");
        return 0;
    }
    if (errno != ENOENT) {
        snprintf(error, error_size, "Cannot access results directory: %s", strerror(errno));
        return 0;
    }
#ifdef _WIN32
    if (_mkdir(directory) == 0) return 1;
#else
    if (mkdir(directory, 0755) == 0) return 1;
#endif
    snprintf(error, error_size, "Cannot create results directory: %s", strerror(errno));
    return 0;
}

int initialize_results(char *directory, size_t capacity, char *error, size_t error_size)
{
    char root[RESULT_PATH_CAPACITY];
#ifdef _WIN32
    char *last;
    DWORD length = GetModuleFileNameA(NULL, root, (DWORD)sizeof(root));
    if (!length || length >= sizeof(root)) {
        snprintf(error, error_size, "Cannot locate the executable directory.");
        return 0;
    }
    last = strrchr(root, '\\');
    if (!last) {
        snprintf(error, error_size, "Cannot locate the executable directory.");
        return 0;
    }
    *last = '\0';
    last = strrchr(root, '\\');
    /* build/memory_policy_lab.exe -> project/results. */
    if (last && compare_names(last + 1, "build") == 0) *last = '\0';
#else
    /* Unix make/run commands are documented relative to the project root. */
    if (!getcwd(root, sizeof(root))) {
        snprintf(error, error_size, "Cannot locate the working directory: %s", strerror(errno));
        return 0;
    }
#endif
    if (!join_path(root, "results", directory, capacity, error, error_size)) return 0;
    return ensure_results_directory(directory, error, error_size);
}

int resolve_reference_path(const char *entry, const char *directory,
                           char *path, size_t capacity, char *error, size_t error_size)
{
    int length;
    if (!entry || !entry[0]) {
        snprintf(error, error_size, "A file name or path is required.");
        return 0;
    }
    if (strchr(entry, '/') || strchr(entry, '\\') || strchr(entry, ':')) {
        length = snprintf(path, capacity, "%s", entry);
        if (length < 0 || (size_t)length >= capacity) {
            snprintf(error, error_size, "The file path is too long.");
            return 0;
        }
        return 1;
    }
    /* Bare names are result inputs, always saved with a usable .txt extension. */
    {
        char name[RESULT_NAME_CAPACITY];
        length = snprintf(name, sizeof(name), "%s%s", entry, text_extension(entry) ? "" : ".txt");
        if (length < 0 || (size_t)length >= sizeof(name)) {
            snprintf(error, error_size, "The result file name is too long.");
            return 0;
        }
        return join_path(directory, name, path, capacity, error, error_size);
    }
}

int next_result_path(const char *directory, char *path, size_t capacity,
                     char *error, size_t error_size)
{
    unsigned long number;
    for (number = 1; number <= 999999; ++number) {
        char name[40];
        struct stat info;
        snprintf(name, sizeof(name), "result_%03lu.txt", number);
        if (!join_path(directory, name, path, capacity, error, error_size)) return 0;
        if (stat(path, &info) != 0) {
            if (errno == ENOENT) return 1;
            snprintf(error, error_size, "Cannot check result file: %s", strerror(errno));
            return 0;
        }
    }
    snprintf(error, error_size, "No automatic result file name is available.");
    return 0;
}

static void catalog_file(const char *directory, const char *name, ResultCatalog *catalog)
{
    char path[RESULT_PATH_CAPACITY], error[256];
    ReferenceString references;
    struct stat info;
    if (!text_extension(name)) return;
    if (!join_path(directory, name, path, sizeof(path), error, sizeof(error))) return;
    if (stat(path, &info) != 0) return;
#ifdef _WIN32
    if ((info.st_mode & S_IFMT) != S_IFREG) return;
#else
    if (!S_ISREG(info.st_mode)) return;
#endif
    if (!load_references(path, &references, error, sizeof(error))) {
        ++catalog->invalid;
        return;
    }
    if (catalog->count == MAX_RESULT_FILES || strlen(name) >= RESULT_NAME_CAPACITY) {
        ++catalog->omitted;
        return;
    }
    strcpy(catalog->files[catalog->count].name, name);
    catalog->files[catalog->count].references = references.count;
    ++catalog->count;
}

static int compare_files(const void *a, const void *b)
{
    const ResultFile *left = a, *right = b;
    int difference = compare_names(left->name, right->name);
    if (difference) return difference;
    return strcmp(left->name, right->name);
}

int list_result_files(const char *directory, ResultCatalog *catalog,
                      char *error, size_t error_size)
{
    memset(catalog, 0, sizeof(*catalog));
    if (!ensure_results_directory(directory, error, error_size)) return 0;
#ifdef _WIN32
    {
        char pattern[RESULT_PATH_CAPACITY];
        struct _finddata_t data;
        intptr_t handle;
        int enumeration_error;
        if (!join_path(directory, "*", pattern, sizeof(pattern), error, error_size)) return 0;
        handle = _findfirst(pattern, &data);
        if (handle == -1) {
            if (errno == ENOENT) return 1;
            snprintf(error, error_size, "Cannot list result files: %s", strerror(errno));
            return 0;
        }
        do {
            if (!(data.attrib & _A_SUBDIR)) catalog_file(directory, data.name, catalog);
        } while (_findnext(handle, &data) == 0);
        enumeration_error = errno;
        _findclose(handle);
        if (enumeration_error != ENOENT) {
            snprintf(error, error_size, "Could not finish listing result files: %s", strerror(enumeration_error));
            return 0;
        }
    }
#else
    {
        DIR *directory_stream = opendir(directory);
        struct dirent *entry;
        int enumeration_error;
        if (!directory_stream) {
            snprintf(error, error_size, "Cannot list result files: %s", strerror(errno));
            return 0;
        }
        for (;;) {
            errno = 0;
            entry = readdir(directory_stream);
            if (!entry) break;
            catalog_file(directory, entry->d_name, catalog);
        }
        enumeration_error = errno;
        closedir(directory_stream);
        if (enumeration_error) {
            snprintf(error, error_size, "Could not finish listing result files: %s", strerror(enumeration_error));
            return 0;
        }
    }
#endif
    qsort(catalog->files, catalog->count, sizeof(catalog->files[0]), compare_files);
    return 1;
}
