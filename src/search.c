#include "search.h"

#include <string.h>

const char *search_condition_name(SearchCondition condition)
{
    switch (condition) {
        case FIFO_BEATS_LRU: return "FIFO faults < LRU faults";
        case LRU_BEATS_FIFO: return "LRU faults < FIFO faults";
        case FIFO_BELADY: return "FIFO faults increase with one extra frame";
        default: return "Unknown condition";
    }
}

int search_candidate_count(const SearchConfig *config, unsigned long *total)
{
    int i;
    unsigned long count = 1;
    if (!config || !total || config->page_count < 1 ||
        config->page_count > SEARCH_MAX_PAGES || config->length < 1 ||
        config->length > SEARCH_MAX_LENGTH || config->frame_count < 1 ||
        config->frame_count > MAX_FRAMES || config->limit < 1 ||
        config->limit > SEARCH_MAX_CANDIDATES ||
        config->condition < FIFO_BEATS_LRU || config->condition > FIFO_BELADY ||
        (config->condition == FIFO_BELADY && config->frame_count == MAX_FRAMES))
        return 0;
    /* 6^12 = 2,176,782,336 fits even a 32-bit unsigned long. */
    for (i = 0; i < config->length; ++i) count *= (unsigned long)config->page_count;
    *total = count;
    return 1;
}

static void next_candidate(ReferenceString *references, int page_count)
{
    size_t position = references->count;
    /* Count in base page_count, incrementing the rightmost digit first. */
    while (position > 0) {
        --position;
        if (++references->pages[position] < page_count) return;
        references->pages[position] = 0;
    }
}

SearchStatus search_references(const SearchConfig *config, SearchResult *result)
{
    ReferenceString candidate = {{0}, 0};
    unsigned long budget;
    if (!result) return SEARCH_INVALID;
    memset(result, 0, sizeof(*result));
    if (!search_candidate_count(config, &result->total)) return SEARCH_INVALID;
    candidate.count = (size_t)config->length;
    budget = config->limit < result->total ? config->limit : result->total;
    while (result->tested < budget) {
        int matches;
        simulate(&candidate, config->frame_count, FIFO, NULL, NULL, &result->left);
        simulate(&candidate,
                 config->frame_count + (config->condition == FIFO_BELADY ? 1 : 0),
                 config->condition == FIFO_BELADY ? FIFO : LRU,
                 NULL, NULL, &result->right);
        ++result->tested;
        if (config->condition == FIFO_BEATS_LRU)
            matches = result->left.faults < result->right.faults;
        else if (config->condition == LRU_BEATS_FIFO)
            matches = result->right.faults < result->left.faults;
        else matches = result->right.faults > result->left.faults;
        if (matches) {
            result->references = candidate;
            result->status = SEARCH_FOUND;
            return result->status;
        }
        next_candidate(&candidate, config->page_count);
    }
    memset(&result->left, 0, sizeof(result->left));
    memset(&result->right, 0, sizeof(result->right));
    result->status = result->tested == result->total
        ? SEARCH_EXHAUSTED : SEARCH_LIMIT_REACHED;
    return result->status;
}
