#include "search.h"
#include "input.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

/* Independent candidate construction: decode an integer instead of advancing an array. */
static SearchStatus oracle(const SearchConfig *config, SearchResult *result)
{
    unsigned long code, total;
    memset(result, 0, sizeof(*result));
    assert(search_candidate_count(config, &total));
    result->total = total;
    for (code = 0; code < total && code < config->limit; ++code) {
        ReferenceString refs = {{0}, (size_t)config->length};
        unsigned long n = code;
        int position, match;
        Stats left, right;
        for (position = config->length - 1; position >= 0; --position) {
            refs.pages[position] = (int)(n % (unsigned long)config->page_count);
            n /= (unsigned long)config->page_count;
        }
        assert(simulate(&refs, config->frame_count, FIFO, NULL, NULL, &left));
        assert(simulate(&refs, config->frame_count + (config->condition == FIFO_BELADY),
                        config->condition == FIFO_BELADY ? FIFO : LRU, NULL, NULL, &right));
        ++result->tested;
        if (config->condition == FIFO_BEATS_LRU) match = left.faults < right.faults;
        else if (config->condition == LRU_BEATS_FIFO) match = right.faults < left.faults;
        else match = right.faults > left.faults;
        if (match) {
            result->references = refs;
            result->left = left;
            result->right = right;
            return result->status = SEARCH_FOUND;
        }
    }
    return result->status = result->tested == total ? SEARCH_EXHAUSTED : SEARCH_LIMIT_REACHED;
}

static void check_search(const SearchConfig *config)
{
    SearchResult actual, expected;
    size_t i;
    assert(search_references(config, &actual) == oracle(config, &expected));
    assert(actual.status == expected.status);
    assert(actual.total == expected.total && actual.tested == expected.tested);
    assert(actual.references.count == expected.references.count);
    for (i = 0; i < actual.references.count; ++i)
        assert(actual.references.pages[i] == expected.references.pages[i]);
    assert(actual.left.faults == expected.left.faults);
    assert(actual.right.faults == expected.right.faults);
    assert(actual.tested <= config->limit && actual.tested <= actual.total);
}

static void test_round_trip(const ReferenceString *references)
{
    ReferenceString loaded = {{0}, 0};
    ReferenceString empty = {{0}, 0};
    char error[256];
    Stats before, after;
    size_t i;
    assert(save_references("build/search-result.tmp", references, error, sizeof(error)));
    assert(load_references("build/search-result.tmp", &loaded, error, sizeof(error)));
    assert(loaded.count == references->count);
    for (i = 0; i < loaded.count; ++i) assert(loaded.pages[i] == references->pages[i]);
    assert(simulate(references, 2, FIFO, NULL, NULL, &before));
    assert(simulate(&loaded, 2, FIFO, NULL, NULL, &after));
    assert(before.faults == after.faults && before.hits == after.hits);
    assert(remove("build/search-result.tmp") == 0);
    assert(!save_references("build/unused.tmp", &empty, error, sizeof(error)));
    assert(!save_references("build/nonexistent-directory/result.txt", references, error, sizeof(error)));
}

int main(void)
{
    SearchConfig config = {3, 5, 2, SEARCH_MAX_CANDIDATES, LRU_BEATS_FIFO};
    SearchResult result;
    unsigned long total;
    int pages, length, frames, condition;

    assert(search_references(&config, &result) == SEARCH_FOUND);
    assert(result.tested == 34 && result.total == 243);
    assert(result.left.faults == 4 && result.right.faults == 3);
    test_round_trip(&result.references);
    /* A match exactly at the budget boundary must still be returned. */
    config.limit = 34;
    assert(search_references(&config, &result) == SEARCH_FOUND);
    config.limit = 33;
    assert(search_references(&config, &result) == SEARCH_LIMIT_REACHED);
    assert(result.tested == 33 && result.references.count == 0);
    config = (SearchConfig){4, 7, 3, SEARCH_MAX_CANDIDATES, FIFO_BEATS_LRU};
    assert(search_references(&config, &result) == SEARCH_FOUND);
    assert(result.left.faults < result.right.faults);
    check_search(&config);
    config = (SearchConfig){3, 5, 1, 243, FIFO_BEATS_LRU};
    assert(search_references(&config, &result) == SEARCH_EXHAUSTED);
    assert(result.tested == 243);
    config.limit = 242;
    assert(search_references(&config, &result) == SEARCH_LIMIT_REACHED);
    config = (SearchConfig){6, 12, 9, SEARCH_MAX_CANDIDATES, FIFO_BELADY};
    assert(search_candidate_count(&config, &total) && total == 2176782336UL);

    for (pages = 1; pages <= 3; ++pages)
        for (length = 1; length <= 5; ++length)
            for (frames = 1; frames <= 4; ++frames)
                for (condition = FIFO_BEATS_LRU; condition <= FIFO_BELADY; ++condition) {
                    config = (SearchConfig){pages, length, frames, SEARCH_MAX_CANDIDATES,
                                             (SearchCondition)condition};
                    check_search(&config);
                    config.limit = 2;
                    check_search(&config);
                }

    config = (SearchConfig){3, 5, 2, SEARCH_MAX_CANDIDATES, LRU_BEATS_FIFO};
    config.page_count = 0;
    assert(search_references(&config, &result) == SEARCH_INVALID);
    config.page_count = 7;
    assert(search_references(&config, &result) == SEARCH_INVALID);
    config.page_count = 3; config.length = 13;
    assert(search_references(&config, &result) == SEARCH_INVALID);
    config.length = 5; config.limit = 0;
    assert(search_references(&config, &result) == SEARCH_INVALID);
    config.limit = SEARCH_MAX_CANDIDATES + 1;
    assert(search_references(&config, &result) == SEARCH_INVALID);
    config.limit = SEARCH_MAX_CANDIDATES; config.condition = FIFO_BELADY; config.frame_count = 10;
    assert(search_references(&config, &result) == SEARCH_INVALID);
    assert(search_references(NULL, &result) == SEARCH_INVALID);
    assert(search_references(&config, NULL) == SEARCH_INVALID);
    puts("Search tests passed: both winners, enumeration oracle, bounds, budget boundaries, save/reload.");
    return 0;
}
