#include "simulator.h"

#include <stdio.h>
#include <string.h>

const char *algorithm_name(Algorithm algorithm)
{
    switch (algorithm) {
        case FIFO: return "FIFO";
        case LRU: return "LRU";
        case OPTIMAL: return "Optimal";
        default: return "Unknown";
    }
}

int simulate(const ReferenceString *references, int frame_count,
             Algorithm algorithm, StepCallback callback, void *context,
             Stats *stats)
{
    int frames[MAX_FRAMES];
    size_t last_used[MAX_FRAMES] = {0};
    int fifo_next = 0;
    size_t i;
    int j;

    if (!stats) return 0;
    memset(stats, 0, sizeof(*stats));
    if (!references || references->count == 0 ||
        references->count > MAX_REFERENCES || frame_count < 1 ||
        frame_count > MAX_FRAMES || algorithm < FIFO ||
        algorithm >= ALGORITHM_COUNT) return 0;
    for (i = 0; i < references->count; ++i)
        if (references->pages[i] < 0) return 0;
    for (j = 0; j < MAX_FRAMES; ++j) frames[j] = EMPTY_PAGE;

    for (i = 0; i < references->count; ++i) {
        Step step;
        int slot = -1;
        int page = references->pages[i];
        memset(&step, 0, sizeof(step));
        step.index = i;
        step.page = page;
        step.frame_count = frame_count;
        step.evicted_page = EMPTY_PAGE;

        for (j = 0; j < frame_count; ++j)
            if (frames[j] == page) { slot = j; break; }

        if (slot >= 0) {
            ++stats->hits;
            snprintf(step.reason, sizeof(step.reason),
                     "Page %d is already in frame %d.", page, slot + 1);
        } else {
            step.fault = 1;
            ++stats->faults;
            for (j = 0; j < frame_count; ++j)
                if (frames[j] == EMPTY_PAGE) { slot = j; break; }

            if (slot >= 0) {
                snprintf(step.reason, sizeof(step.reason),
                         "Load page %d into empty frame %d; no eviction.",
                         page, slot + 1);
            } else {
                if (algorithm == FIFO) {
                    slot = fifo_next;
                    snprintf(step.reason, sizeof(step.reason),
                             "Page %d entered memory first (FIFO).", frames[slot]);
                } else if (algorithm == LRU) {
                    slot = 0;
                    for (j = 1; j < frame_count; ++j)
                        if (last_used[j] < last_used[slot]) slot = j;
                    snprintf(step.reason, sizeof(step.reason),
                             "Page %d was least recently used, at step %u (LRU).",
                             frames[slot], (unsigned)(last_used[slot] + 1));
                } else {
                    size_t farthest = 0;
                    slot = 0;
                    for (j = 0; j < frame_count; ++j) {
                        size_t next;
                        for (next = i + 1; next < references->count; ++next)
                            if (references->pages[next] == frames[j]) break;
                        if (j == 0 || next > farthest) {
                            farthest = next;
                            slot = j;
                        }
                    }
                    if (farthest == references->count)
                        snprintf(step.reason, sizeof(step.reason),
                                 "Page %d is never used again (Optimal).", frames[slot]);
                    else
                        snprintf(step.reason, sizeof(step.reason),
                                 "Page %d is used farthest in the future, at step %u (Optimal).",
                                 frames[slot], (unsigned)(farthest + 1));
                }
                step.evicted_page = frames[slot];
                ++stats->replacements;
            }
            frames[slot] = page;
            /* Hits must not advance the FIFO queue. */
            if (algorithm == FIFO) fifo_next = (slot + 1) % frame_count;
        }

        last_used[slot] = i;
        step.slot = slot;
        memcpy(step.frames, frames, sizeof(frames));
        if (callback && !callback(&step, context)) break;
    }
    return 1;
}
