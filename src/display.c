#include "display.h"
#include "input.h"

#include <stdio.h>

typedef struct { int interactive; } DisplayContext;

void show_references(const ReferenceString *references)
{
    size_t i;
    printf("References (%u):\n", (unsigned)references->count);
    for (i = 0; i < references->count; ++i)
        printf("%d%s", references->pages[i],
               (i + 1) % 20 == 0 || i + 1 == references->count ? "\n" : " ");
}

static int show_step(const Step *step, void *context)
{
    DisplayContext *display = context;
    int j;
    printf("%4u %10d |", (unsigned)(step->index + 1), step->page);
    for (j = 0; j < step->frame_count; ++j) {
        if (step->frames[j] == EMPTY_PAGE) printf(" %10s ", "-");
        else printf(" %10d%c", step->frames[j],
                    step->fault && j == step->slot ? '*' : ' ');
    }
    printf("| %-5s | ", step->fault ? "FAULT" : "HIT");
    if (step->evicted_page == EMPTY_PAGE) puts("-");
    else printf("%d -> %d\n", step->evicted_page, step->page);
    printf("     %s\n", step->reason);
    if (display->interactive) {
        char line[32];
        int status;
        for (;;) {
            printf("[Enter] next  [a] run remaining  [q] stop: ");
            fflush(stdout);
            status = read_line(line, sizeof(line));
            if (status == 0) return 0;
            if (status == 1) {
                if (line[0] == '\0') return 1;
                if ((line[0] == 'q' || line[0] == 'Q') && line[1] == '\0') return 0;
                if ((line[0] == 'a' || line[0] == 'A') && line[1] == '\0') {
                    display->interactive = 0;
                    return 1;
                }
            }
            puts("Please press Enter, a, or q.");
        }
    }
    return 1;
}

void run_display(const ReferenceString *references, int frame_count,
                 Algorithm algorithm, int interactive)
{
    DisplayContext context = {interactive};
    Stats stats;
    int j;
    printf("\n%s | %d frames | * = loaded/changed frame\n", algorithm_name(algorithm), frame_count);
    printf("Step       Page |");
    for (j = 0; j < frame_count; ++j) printf("   Frame %-2d ", j + 1);
    puts("| Result | Evicted -> Incoming");
    if (!simulate(references, frame_count, algorithm, show_step, &context, &stats)) {
        puts("Invalid simulation configuration.");
        return;
    }
    printf("\n%s: %u/%u references processed\n",
           stats.hits + stats.faults == references->count ? "Complete" : "Stopped",
           (unsigned)(stats.hits + stats.faults), (unsigned)references->count);
    printf("Hits: %u | Faults: %u | Replacements: %u | Fault rate: %.2f%%\n",
           (unsigned)stats.hits, (unsigned)stats.faults, (unsigned)stats.replacements,
           100.0 * (double)stats.faults / (double)(stats.hits + stats.faults));
}

void compare_algorithms(const ReferenceString *references, int frame_count)
{
    int a;
    printf("\nComparison: same %u references, %d frames, empty initial memory\n",
           (unsigned)references->count, frame_count);
    puts("Algorithm       Hits    Faults  Replacements  Fault rate");
    for (a = FIFO; a < ALGORITHM_COUNT; ++a) {
        Stats stats;
        simulate(references, frame_count, (Algorithm)a, NULL, NULL, &stats);
        printf("%-10s %9u %9u %13u %10.2f%%\n", algorithm_name((Algorithm)a),
               (unsigned)stats.hits, (unsigned)stats.faults, (unsigned)stats.replacements,
               100.0 * (double)stats.faults / (double)references->count);
    }
    puts("Optimal uses future references and serves as a theoretical benchmark.");
}

void frame_sweep(const ReferenceString *references)
{
    int frames, a;
    size_t previous_fifo = 0;
    puts("\nFault counts by frame count (each simulation starts empty)");
    puts("Frames       FIFO        LRU    Optimal");
    for (frames = 1; frames <= MAX_FRAMES; ++frames) {
        size_t fifo_faults = 0;
        printf("%6d", frames);
        for (a = FIFO; a < ALGORITHM_COUNT; ++a) {
            Stats stats;
            simulate(references, frames, (Algorithm)a, NULL, NULL, &stats);
            printf(" %10u", (unsigned)stats.faults);
            if (a == FIFO) fifo_faults = stats.faults;
        }
        if (frames > 1 && fifo_faults > previous_fifo)
            printf("  <- FIFO Belady's anomaly: %u -> %u", (unsigned)previous_fifo, (unsigned)fifo_faults);
        puts("");
        previous_fifo = fifo_faults;
    }
}
