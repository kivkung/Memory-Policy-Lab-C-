#ifndef SIMULATOR_H
#define SIMULATOR_H

#include <stddef.h>

#define MAX_FRAMES 10
#define MAX_REFERENCES 1000
#define EMPTY_PAGE (-1)

typedef enum { FIFO, LRU, OPTIMAL, ALGORITHM_COUNT } Algorithm;

typedef struct {
    int pages[MAX_REFERENCES];
    size_t count;
} ReferenceString;

/* One complete snapshot after accessing a page. No pointers to mutable frames. */
typedef struct {
    size_t index;
    int page;
    int frames[MAX_FRAMES];
    int frame_count;
    int slot;
    int fault;
    int evicted_page;
    char reason[180];
} Step;

typedef struct {
    size_t hits;
    size_t faults;
    size_t replacements;
} Stats;

/* Return nonzero to continue, zero to stop after this step. */
typedef int (*StepCallback)(const Step *step, void *context);

const char *algorithm_name(Algorithm algorithm);
int simulate(const ReferenceString *references, int frame_count,
             Algorithm algorithm, StepCallback callback, void *context,
             Stats *stats);

#endif
