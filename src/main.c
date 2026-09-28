#include "display.h"
#include "input.h"

#include <stdio.h>
#include <string.h>

int main(int argc, char **argv)
{
    ReferenceString references = {{0}, 0};
    int frames = 3;
    Algorithm algorithm = FIFO;
    char line[1024];
    char error[256];

    if (argc > 2) {
        puts("Usage: simulator [reference-file.txt]");
        return 1;
    }
    if (argc == 2) {
        if (!load_references(argv[1], &references, error, sizeof(error))) {
            fprintf(stderr, "%s\n", error);
            return 1;
        }
        printf("Loaded %u references.\n", (unsigned)references.count);
    }

    for (;;) {
        int choice;
        int status;
        printf("\n=== Page Replacement Simulator ===\n");
        printf("Frames: %d | Algorithm: %s | References: %u\n",
               frames, algorithm_name(algorithm), (unsigned)references.count);
        puts("1. Load references from .txt file\n"
             "2. Set number of frames\n"
             "3. Select algorithm\n"
             "4. Run step by step (fresh start)\n"
             "5. Run all (fresh start)\n"
             "6. Compare algorithms\n"
             "7. Show reference string\n"
             "8. Compare frame counts / Belady's anomaly\n"
             "0. Exit");
        printf("Choose: ");
        fflush(stdout);
        status = read_line(line, sizeof(line));
        if (status == 0) break;
        if (status < 0 || !parse_integer(line, 0, 8, &choice)) {
            puts("Please enter a menu number from 0 to 8.");
            continue;
        }
        if (choice == 0) break;
        if (choice == 1) {
            size_t length;
            printf("File path (spaces allowed; quotes optional): ");
            fflush(stdout);
            status = read_line(line, sizeof(line));
            if (status == 0) break;
            if (status < 0) { puts("Path is too long."); continue; }
            length = strlen(line);
            if (length >= 2 && line[0] == '"' && line[length - 1] == '"') {
                line[length - 1] = '\0';
                memmove(line, line + 1, length - 1);
            }
            if (load_references(line, &references, error, sizeof(error)))
                printf("Loaded %u references.\n", (unsigned)references.count);
            else printf("Load failed: %s\nPrevious references are unchanged.\n", error);
        } else if (choice == 2 || choice == 3) {
            int value;
            if (choice == 2) printf("Number of frames (1-%d): ", MAX_FRAMES);
            else printf("Algorithm: 1 = FIFO, 2 = LRU, 3 = Optimal: ");
            fflush(stdout);
            status = read_line(line, sizeof(line));
            if (status == 0) break;
            if (status < 0 || !parse_integer(line, 1, choice == 2 ? MAX_FRAMES : 3, &value)) {
                puts("Invalid value; previous setting is unchanged.");
                continue;
            }
            if (choice == 2) frames = value;
            else algorithm = (Algorithm)(value - 1);
        } else if (references.count == 0) {
            puts("Load a reference file first (option 1).");
        } else if (choice == 4 || choice == 5) {
            run_display(&references, frames, algorithm, choice == 4);
        } else if (choice == 6) {
            compare_algorithms(&references, frames);
        } else if (choice == 7) {
            show_references(&references);
        } else if (choice == 8) {
            frame_sweep(&references);
        }
    }
    puts("Goodbye.");
    return 0;
}
