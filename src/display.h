#ifndef DISPLAY_H
#define DISPLAY_H

#include "simulator.h"

void show_references(const ReferenceString *references);
void run_display(const ReferenceString *references, int frame_count,
                 Algorithm algorithm, int interactive);
void compare_algorithms(const ReferenceString *references, int frame_count);
void frame_sweep(const ReferenceString *references);

#endif
