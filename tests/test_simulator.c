#include "simulator.h"
#include "input.h"

#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>

typedef struct {
    int before[MAX_FRAMES];
    size_t count;
} TraceCheck;

static int check_trace(const Step *step, void *context)
{
    TraceCheck *check = context;
    int j, k, found = 0;
    for (j = 0; j < step->frame_count; ++j)
        if (check->before[j] == step->page) found = 1;
    assert(step->fault == !found);
    assert(step->index == check->count++);
    assert(step->slot >= 0 && step->slot < step->frame_count);
    assert(step->frames[step->slot] == step->page);
    assert(step->reason[0]);
    for (j = 0; j < step->frame_count; ++j) {
        if (!step->fault || j != step->slot)
            assert(step->frames[j] == check->before[j]);
        for (k = j + 1; k < step->frame_count; ++k)
            assert(step->frames[j] == EMPTY_PAGE || step->frames[j] != step->frames[k]);
    }
    assert(step->evicted_page == (step->fault ? check->before[step->slot] : EMPTY_PAGE));
    memcpy(check->before, step->frames, sizeof(check->before));
    return 1;
}

static Stats run(const ReferenceString *refs, int frames, Algorithm algorithm)
{
    TraceCheck check;
    Stats stats;
    int i;
    check.count = 0;
    for (i = 0; i < MAX_FRAMES; ++i) check.before[i] = EMPTY_PAGE;
    assert(simulate(refs, frames, algorithm, check_trace, &check, &stats));
    assert(check.count == refs->count);
    assert(stats.hits + stats.faults == refs->count);
    return stats;
}

static int stop_first(const Step *step, void *context)
{
    (void)step;
    (void)context;
    return 0;
}

/* Independent exhaustive search: minimum achievable faults on short traces. */
static size_t minimum_faults(const ReferenceString *refs, size_t index,
                             int *frames, int frame_count)
{
    size_t best = refs->count + 1;
    int j;
    if (index == refs->count) return 0;
    for (j = 0; j < frame_count; ++j)
        if (frames[j] == refs->pages[index])
            return minimum_faults(refs, index + 1, frames, frame_count);
    for (j = 0; j < frame_count; ++j) {
        int old = frames[j];
        size_t cost;
        frames[j] = refs->pages[index];
        cost = 1 + minimum_faults(refs, index + 1, frames, frame_count);
        frames[j] = old;
        if (cost < best) best = cost;
    }
    return best;
}

static void test_algorithms(void)
{
    ReferenceString classic = {{7,0,1,2,0,3,0,4,2,3,0,3,2}, 13};
    ReferenceString belady = {{1,2,3,4,1,2,5,1,2,3,4,5}, 12};
    ReferenceString repeated = {{0,0,0,0,0}, 5};
    ReferenceString hit_order = {{1,2,1,3,1}, 5};
    ReferenceString limits = {{0,INT_MAX,0,INT_MAX}, 4};
    ReferenceString empty = {{0}, 0};
    Stats stats;
    int a, f, code;

    assert(run(&classic, 3, FIFO).faults == 10);
    assert(run(&classic, 3, LRU).faults == 9);
    assert(run(&classic, 3, OPTIMAL).faults == 7);
    assert(run(&belady, 3, FIFO).faults == 9);
    assert(run(&belady, 4, FIFO).faults == 10);
    assert(run(&hit_order, 2, FIFO).faults == 4);
    assert(run(&hit_order, 2, LRU).faults == 3);
    for (a = FIFO; a < ALGORITHM_COUNT; ++a) {
        for (f = 1; f <= MAX_FRAMES; ++f) {
            stats = run(&repeated, f, (Algorithm)a);
            assert(stats.faults == 1 && stats.hits == 4 && stats.replacements == 0);
        }
        assert(run(&classic, 1, (Algorithm)a).faults == 13);
        assert(run(&classic, 10, (Algorithm)a).faults == 6);
        assert(run(&limits, 2, (Algorithm)a).faults == 2);
    }
    assert(!simulate(&empty, 3, FIFO, NULL, NULL, &stats));
    assert(!simulate(&classic, 0, FIFO, NULL, NULL, &stats));
    assert(!simulate(&classic, 11, FIFO, NULL, NULL, &stats));
    assert(!simulate(&classic, 3, ALGORITHM_COUNT, NULL, NULL, &stats));
    assert(simulate(&classic, 3, FIFO, stop_first, NULL, &stats));
    assert(stats.faults == 1 && stats.hits == 0);

    /* All 729 length-six traces over pages 0,1,2, with 1..3 frames. */
    for (code = 0; code < 729; ++code) {
        ReferenceString refs = {{0}, 6};
        int n = code;
        size_t i;
        size_t previous_lru = 7, previous_opt = 7;
        for (i = 0; i < refs.count; ++i) { refs.pages[i] = n % 3; n /= 3; }
        for (f = 1; f <= 3; ++f) {
            int oracle_frames[3] = {EMPTY_PAGE, EMPTY_PAGE, EMPTY_PAGE};
            Stats fifo = run(&refs, f, FIFO);
            Stats lru = run(&refs, f, LRU);
            Stats opt = run(&refs, f, OPTIMAL);
            assert(opt.faults == minimum_faults(&refs, 0, oracle_frames, f));
            assert(opt.faults <= fifo.faults && opt.faults <= lru.faults);
            assert(lru.faults <= previous_lru && opt.faults <= previous_opt);
            previous_lru = lru.faults;
            previous_opt = opt.faults;
        }
    }
}

static void check_file(const unsigned char *content, size_t length, int valid, size_t count)
{
    const char *path = "build/test-input.tmp";
    FILE *file = fopen(path, "wb");
    ReferenceString refs = {{42}, 1};
    char error[256];
    assert(file);
    assert(fwrite(content, 1, length, file) == length);
    assert(fclose(file) == 0);
    assert(load_references(path, &refs, error, sizeof(error)) == valid);
    if (valid) assert(refs.count == count);
    else assert(refs.count == 1 && refs.pages[0] == 42 && error[0]);
    assert(remove(path) == 0);
}

static void test_input(void)
{
    static const unsigned char bom[] = {0xef,0xbb,0xbf,'0',' ','1','\r','\n','2'};
    static const unsigned char utf16[] = {0xff,0xfe,'1',0};
    static const unsigned char nul[] = {'1',0,'2'};
    char buffer[2 * (MAX_REFERENCES + 1)];
    const char *invalid[] = {"", " \r\n\t", "1 two 3", "-1", "1,2", "1.5", "+1", "99999999999999999999"};
    ReferenceString refs = {{0}, 0};
    char error[256];
    int value;
    size_t i;
    check_file(bom, sizeof(bom), 1, 3);
    check_file(utf16, sizeof(utf16), 0, 0);
    check_file(nul, sizeof(nul), 0, 0);
    check_file((const unsigned char *)"0\t12\r\n3", 7, 1, 3);
    for (i = 0; i < sizeof(invalid) / sizeof(invalid[0]); ++i)
        check_file((const unsigned char *)invalid[i], strlen(invalid[i]), 0, 0);
    for (i = 0; i < sizeof(buffer); ++i) buffer[i] = i % 2 ? ' ' : '1';
    check_file((const unsigned char *)buffer, 2 * MAX_REFERENCES, 1, MAX_REFERENCES);
    check_file((const unsigned char *)buffer, sizeof(buffer), 0, 0);
    assert(!load_references("build/does-not-exist.txt", &refs, error, sizeof(error)));
    assert(parse_integer(" 3 ", 1, 10, &value) && value == 3);
    assert(!parse_integer("3x", 1, 10, &value));
    assert(!parse_integer("0", 1, 10, &value));
    assert(!parse_integer("99999999999999999999", 1, 10, &value));
}

int main(void)
{
    test_algorithms();
    test_input();
    puts("All tests passed (known results, trace invariants, exhaustive Optimal oracle, input validation).");
    return 0;
}
