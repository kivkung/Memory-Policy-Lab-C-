#ifndef SEARCH_H
#define SEARCH_H

#include "simulator.h"

#define SEARCH_MAX_PAGES 6
#define SEARCH_MAX_LENGTH 12
#define SEARCH_MAX_CANDIDATES 100000UL

typedef enum { FIFO_BEATS_LRU = 1, LRU_BEATS_FIFO, FIFO_BELADY } SearchCondition;

typedef struct {
    int page_count;       /* Candidate page IDs: 0 .. page_count - 1. */
    int length;          /* Search exactly this length. */
    int frame_count;     /* Belady compares F with F + 1. */
    unsigned long limit;
    SearchCondition condition;
} SearchConfig;

typedef enum {
    SEARCH_INVALID, SEARCH_FOUND, SEARCH_EXHAUSTED, SEARCH_LIMIT_REACHED
} SearchStatus;

typedef struct {
    SearchStatus status;
    unsigned long total;
    unsigned long tested;
    ReferenceString references;
    Stats left;
    Stats right;
} SearchResult;

const char *search_condition_name(SearchCondition condition);
int search_candidate_count(const SearchConfig *config, unsigned long *total);
SearchStatus search_references(const SearchConfig *config, SearchResult *result);

#endif
