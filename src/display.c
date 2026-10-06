#include "display.h"
#include "input.h"

#include <stdio.h>
#include <stdlib.h>

#define FRAME_TEXT_SIZE (MAX_FRAMES * 13 + 3)

typedef struct {
    int interactive;
    int width;
    Step *steps;
    size_t count;
    size_t total;
} DisplayContext;

static int frame_width(const ReferenceString *references, int frame_count)
{
    size_t i;
    int digits = 1;
    int width;
    for (i = 0; i < references->count; ++i) {
        char number[16];
        int length = snprintf(number, sizeof(number), "%d", references->pages[i]);
        if (length > digits) digits = length;
    }
    width = 2 + frame_count * digits + 2 * (frame_count - 1);
    return width < 20 ? 20 : width;
}

/* Capacity covers ten INT_MAX page IDs, separators and brackets. */
static void format_frames(const Step *step, char *buffer, size_t capacity)
{
    int i;
    size_t used = 0;
    used += (size_t)snprintf(buffer, capacity, "[");
    for (i = 0; i < step->frame_count; ++i) {
        if (step->frames[i] == EMPTY_PAGE)
            used += (size_t)snprintf(buffer + used, capacity - used, "%s-", i ? ", " : "");
        else
            used += (size_t)snprintf(buffer + used, capacity - used, "%s%d",
                                     i ? ", " : "", step->frames[i]);
    }
    snprintf(buffer + used, capacity - used, "]");
}

static void format_replacement(const Step *step, char *buffer, size_t capacity)
{
    if (step->evicted_page == EMPTY_PAGE) snprintf(buffer, capacity, "None");
    else snprintf(buffer, capacity, "%d -> %d", step->evicted_page, step->page);
}

static int next_action(int *interactive)
{
    char line[32];
    int status;
    if (!*interactive) return 1;
    for (;;) {
        printf("Enter: next    a: run remaining    q: stop\n> ");
        fflush(stdout);
        status = read_line(line, sizeof(line));
        if (status == 0) return 0;
        if (status == 1) {
            if (!line[0]) return 1;
            if ((line[0] == 'q' || line[0] == 'Q') && !line[1]) return 0;
            if ((line[0] == 'a' || line[0] == 'A') && !line[1]) {
                *interactive = 0;
                return 1;
            }
        }
        puts("Please press Enter, a, or q.");
    }
}

static void print_stats(const char *name, const Stats *stats, size_t total)
{
    size_t processed = stats->hits + stats->faults;
    printf("\n%s\n", name);
    printf("  References:    %u / %u\n", (unsigned)processed, (unsigned)total);
    printf("  Hits:          %u\n", (unsigned)stats->hits);
    printf("  Page faults:   %u\n", (unsigned)stats->faults);
    printf("  Replacements:  %u\n", (unsigned)stats->replacements);
    printf("  Fault rate:    %.2f%%\n",
           processed ? 100.0 * (double)stats->faults / (double)processed : 0.0);
}

void show_references(const ReferenceString *references)
{
    size_t i;
    printf("\nReferences (%u):\n\n", (unsigned)references->count);
    for (i = 0; i < references->count; ++i)
        printf("%d%s", references->pages[i],
               (i + 1) % 20 == 0 || i + 1 == references->count ? "\n" : " ");
    puts("");
}

static int show_step(const Step *step, void *context)
{
    DisplayContext *display = context;
    char frames[FRAME_TEXT_SIZE], replacement[32];
    format_frames(step, frames, sizeof(frames));
    format_replacement(step, replacement, sizeof(replacement));
    display->steps[display->count++] = *step;
    printf("%-6u  %10d    %-*s    %-8s    %s\n\n",
           (unsigned)(step->index + 1), step->page, display->width, frames,
           step->fault ? "Fault" : "Hit", replacement);
    if (display->interactive) printf("  Reason: %s\n\n", step->reason);
    return display->count == display->total ? 1 : next_action(&display->interactive);
}

void run_display(const ReferenceString *references, int frame_count,
                 Algorithm algorithm, int interactive)
{
    DisplayContext context;
    Stats stats;
    size_t i;
    char title[40];
    if (!references || !references->count || references->count > MAX_REFERENCES ||
        frame_count < 1 || frame_count > MAX_FRAMES ||
        algorithm < FIFO || algorithm >= ALGORITHM_COUNT) {
        puts("Invalid simulation configuration.");
        return;
    }
    context.interactive = interactive;
    context.width = frame_width(references, frame_count);
    context.count = 0;
    context.total = references->count;
    context.steps = malloc(references->count * sizeof(*context.steps));
    if (!context.steps) { puts("Not enough memory to display the trace."); return; }
    snprintf(title, sizeof(title), "%s after access", algorithm_name(algorithm));
    printf("\n%s    %d frames\n\n", algorithm_name(algorithm), frame_count);
    printf("%-6s  %10s    %-*s    %-8s    %s\n\n",
           "Step", "Page", context.width, title, "Result", "Replaced");
    if (!simulate(references, frame_count, algorithm, show_step, &context, &stats)) {
        free(context.steps);
        puts("Invalid simulation configuration.");
        return;
    }
    printf("%s\n", context.count == references->count ? "Complete" : "Stopped");
    print_stats(algorithm_name(algorithm), &stats, references->count);
    puts("\nReplacement explanations\n");
    {
        int any = 0;
        for (i = 0; i < context.count; ++i) {
            const Step *step = &context.steps[i];
            if (step->evicted_page != EMPTY_PAGE) {
                printf("  Step %u: %d -> %d\n  %s\n\n", (unsigned)(step->index + 1),
                       step->evicted_page, step->page, step->reason);
                any = 1;
            }
        }
        if (!any) puts("  No pages were replaced.\n");
    }
    free(context.steps);
}

void compare_algorithms(const ReferenceString *references, int frame_count)
{
    int a;
    printf("\nSame %u references    %d frames    Empty initial memory\n\n",
           (unsigned)references->count, frame_count);
    printf("%-12s  %8s  %8s  %12s  %12s\n\n",
           "Algorithm", "Hits", "Faults", "Replacements", "Fault rate");
    for (a = FIFO; a < ALGORITHM_COUNT; ++a) {
        Stats stats;
        if (!simulate(references, frame_count, (Algorithm)a, NULL, NULL, &stats)) {
            puts("Invalid simulation configuration.");
            return;
        }
        printf("%-12s  %8u  %8u  %12u  %11.2f%%\n\n", algorithm_name((Algorithm)a),
               (unsigned)stats.hits, (unsigned)stats.faults, (unsigned)stats.replacements,
               100.0 * (double)stats.faults / (double)references->count);
    }
    puts("Optimal uses future references as a theoretical benchmark.\n");
}

void frame_sweep(const ReferenceString *references)
{
    int frames, a;
    size_t previous_fifo = 0;
    puts("\nPage faults by frame count (each run starts empty)\n");
    printf("%-8s  %10s  %10s  %10s    %s\n\n", "Frames", "FIFO", "LRU", "Optimal", "Observation");
    for (frames = 1; frames <= MAX_FRAMES; ++frames) {
        size_t fifo_faults = 0;
        printf("%-8d", frames);
        for (a = FIFO; a < ALGORITHM_COUNT; ++a) {
            Stats stats;
            if (!simulate(references, frames, (Algorithm)a, NULL, NULL, &stats)) {
                puts("Invalid simulation configuration.");
                return;
            }
            printf("  %10u", (unsigned)stats.faults);
            if (a == FIFO) fifo_faults = stats.faults;
        }
        if (frames > 1 && fifo_faults > previous_fifo)
            printf("    FIFO Belady's anomaly: %u -> %u", (unsigned)previous_fifo, (unsigned)fifo_faults);
        puts("\n");
        previous_fifo = fifo_faults;
    }
}

typedef struct { Step *steps; } TraceContext;

static int capture_step(const Step *step, void *context)
{
    TraceContext *trace = context;
    trace->steps[step->index] = *step;
    return 1;
}

static void count_step(const Step *step, Stats *stats)
{
    if (step->fault) ++stats->faults;
    else ++stats->hits;
    if (step->evicted_page != EMPTY_PAGE) ++stats->replacements;
}

void compare_traces(const ReferenceString *references, int frame_count, int interactive)
{
    TraceContext fifo, lru;
    Stats full_stats, fifo_stats = {0, 0, 0}, lru_stats = {0, 0, 0};
    size_t i, shown = 0, first_decision = MAX_REFERENCES, first_outcome = MAX_REFERENCES;
    int width;
    if (!references || !references->count || references->count > MAX_REFERENCES ||
        frame_count < 1 || frame_count > MAX_FRAMES) {
        puts("Invalid simulation configuration.");
        return;
    }
    fifo.steps = malloc(references->count * sizeof(*fifo.steps));
    lru.steps = malloc(references->count * sizeof(*lru.steps));
    if (!fifo.steps || !lru.steps) {
        free(fifo.steps);
        free(lru.steps);
        puts("Not enough memory to compare traces.");
        return;
    }
    if (!simulate(references, frame_count, FIFO, capture_step, &fifo, &full_stats) ||
        !simulate(references, frame_count, LRU, capture_step, &lru, &full_stats)) {
        free(fifo.steps);
        free(lru.steps);
        puts("Invalid simulation configuration.");
        return;
    }
    width = frame_width(references, frame_count);
    printf("\nFIFO and LRU    %d frames    Same input    Empty initial memory\n\n", frame_count);
    printf("%-6s  %10s    %-*s    %-12s    %-*s    %-12s\n\n",
           "Step", "Page", width, "FIFO after access", "FIFO result",
           width, "LRU after access", "LRU result");
    for (i = 0; i < references->count; ++i) {
        char left[FRAME_TEXT_SIZE], right[FRAME_TEXT_SIZE];
        format_frames(&fifo.steps[i], left, sizeof(left));
        format_frames(&lru.steps[i], right, sizeof(right));
        printf("%-6u  %10d    %-*s    %-12s    %-*s    %-12s\n\n",
               (unsigned)(i + 1), references->pages[i], width, left,
               fifo.steps[i].fault ? "Fault" : "Hit", width, right,
               lru.steps[i].fault ? "Fault" : "Hit");
        count_step(&fifo.steps[i], &fifo_stats);
        count_step(&lru.steps[i], &lru_stats);
        ++shown;
        if (first_decision == MAX_REFERENCES && fifo.steps[i].evicted_page != lru.steps[i].evicted_page)
            first_decision = i;
        if (first_outcome == MAX_REFERENCES && fifo.steps[i].fault != lru.steps[i].fault)
            first_outcome = i;
        if (interactive)
            printf("  FIFO: %s\n  LRU:  %s\n\n", fifo.steps[i].reason, lru.steps[i].reason);
        if (i + 1 < references->count && !next_action(&interactive)) break;
    }
    printf("%s: %u/%u steps shown\n", shown == references->count ? "Complete" : "Stopped",
           (unsigned)shown, (unsigned)references->count);
    print_stats("FIFO", &fifo_stats, references->count);
    print_stats("LRU", &lru_stats, references->count);
    puts("\nAnalysis of the shown steps\n");
    if (first_decision != MAX_REFERENCES)
        printf("  First different replacement decision: step %u\n", (unsigned)(first_decision + 1));
    else puts("  No different replacement decisions in the shown steps.");
    if (first_outcome != MAX_REFERENCES)
        printf("  First different Hit/Fault result: step %u\n", (unsigned)(first_outcome + 1));
    else puts("  No different Hit/Fault results in the shown steps.");
    puts("\nReplacement decisions\n");
    {
        int any = 0;
        for (i = 0; i < shown; ++i) {
            if (fifo.steps[i].evicted_page != EMPTY_PAGE || lru.steps[i].evicted_page != EMPTY_PAGE) {
                char left[32], right[32];
                format_replacement(&fifo.steps[i], left, sizeof(left));
                format_replacement(&lru.steps[i], right, sizeof(right));
                printf("  Step %u\n  FIFO: %s    %s\n  LRU:  %s    %s\n\n",
                       (unsigned)(i + 1), left, fifo.steps[i].reason, right, lru.steps[i].reason);
                any = 1;
            }
        }
        if (!any) puts("  No pages were replaced.\n");
    }
    free(fifo.steps);
    free(lru.steps);
}
