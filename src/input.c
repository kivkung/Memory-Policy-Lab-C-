#include "input.h"

#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int parse_integer(const char *text, int minimum, int maximum, int *value)
{
    char *end;
    long number;
    errno = 0;
    number = strtol(text, &end, 10);
    if (text == end || errno == ERANGE || number < minimum || number > maximum)
        return 0;
    while (isspace((unsigned char)*end)) ++end;
    if (*end) return 0;
    *value = (int)number;
    return 1;
}

int read_line(char *buffer, size_t capacity)
{
    size_t length;
    int ch;
    if (!fgets(buffer, (int)capacity, stdin)) return 0;
    length = strlen(buffer);
    if (length && buffer[length - 1] == '\n') {
        buffer[length - 1] = '\0';
        return 1;
    }
    ch = getchar();
    if (ch == EOF || ch == '\n') return 1;
    while ((ch = getchar()) != EOF && ch != '\n') { }
    return -1;
}

int load_references(const char *path, ReferenceString *result,
                    char *error, size_t error_size)
{
    FILE *file = fopen(path, "rb");
    ReferenceString candidate = {{0}, 0};
    unsigned char prefix[3];
    size_t bytes;
    int ch;

    if (!file) {
        snprintf(error, error_size, "Cannot open file: %s", strerror(errno));
        return 0;
    }
    /* Notepad UTF-8 files can have a three-byte BOM. */
    bytes = fread(prefix, 1, sizeof(prefix), file);
    if (bytes >= 2 && ((prefix[0] == 0xff && prefix[1] == 0xfe) ||
                       (prefix[0] == 0xfe && prefix[1] == 0xff))) {
        snprintf(error, error_size, "UTF-16 is unsupported. Save the file as UTF-8.");
        fclose(file);
        return 0;
    }
    if (!(bytes == 3 && prefix[0] == 0xef && prefix[1] == 0xbb && prefix[2] == 0xbf))
        rewind(file);

    while ((ch = fgetc(file)) != EOF) {
        int number = 0;
        if (isspace((unsigned char)ch)) continue;
        if (candidate.count == MAX_REFERENCES) {
            snprintf(error, error_size, "Too many references; maximum is %d.", MAX_REFERENCES);
            fclose(file);
            return 0;
        }
        do {
            if (ch < '0' || ch > '9') {
                snprintf(error, error_size,
                         "Invalid reference #%u. Use nonnegative integers separated by whitespace.",
                         (unsigned)(candidate.count + 1));
                fclose(file);
                return 0;
            }
            if (number > (INT_MAX - (ch - '0')) / 10) {
                snprintf(error, error_size, "Reference #%u exceeds INT_MAX (%d).",
                         (unsigned)(candidate.count + 1), INT_MAX);
                fclose(file);
                return 0;
            }
            number = number * 10 + ch - '0';
            ch = fgetc(file);
        } while (ch != EOF && !isspace((unsigned char)ch));
        candidate.pages[candidate.count++] = number;
    }
    if (ferror(file)) {
        snprintf(error, error_size, "Error while reading the input file.");
        fclose(file);
        return 0;
    }
    fclose(file);
    if (candidate.count == 0) {
        snprintf(error, error_size, "The file contains no references.");
        return 0;
    }
    *result = candidate;
    return 1;
}
