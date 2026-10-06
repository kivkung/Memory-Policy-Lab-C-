#include "display.h"
#include "input.h"
#include "search.h"
#include "results.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int ask_integer(const char *prompt, int minimum, int maximum,
                       int default_value, int *value)
{
    char line[64];
    for (;;) {
        int status;
        printf("%s [%d]: ", prompt, default_value);
        fflush(stdout);
        status = read_line(line, sizeof(line));
        if (!status) return 0;
        if (status == 1 && !line[0]) { *value = default_value; return 1; }
        if (status == 1 && parse_integer(line, minimum, maximum, value)) return 1;
        printf("Please enter an integer from %d to %d, or press Enter for the default.\n",
               minimum, maximum);
    }
}

static void remove_path_quotes(char *path)
{
    size_t length = strlen(path);
    if (length >= 2 && path[0] == '"' && path[length - 1] == '"') {
        path[length - 1] = '\0';
        memmove(path, path + 1, length - 1);
    }
}

static int numeric_selection(const char *text)
{
    if (!*text) return 0;
    while (*text) {
        if (*text < '0' || *text > '9') return 0;
        ++text;
    }
    return 1;
}

static int load_from_results(const char *directory, ReferenceString *references)
{
    ResultCatalog *catalog = malloc(sizeof(*catalog));
    char entry[RESULT_PATH_CAPACITY], path[RESULT_PATH_CAPACITY], error[512];
    size_t i;
    int status, selection;
    if (!catalog) { puts("Not enough memory to list result files."); return 1; }
    printf("\nResults directory:\n%s\n\n", directory);
    if (!list_result_files(directory, catalog, error, sizeof(error))) {
        printf("Cannot list results: %s\nYou can still enter a full path.\n\n", error);
        catalog->count = 0;
    } else {
        puts("Available input files (.txt)\n");
        if (!catalog->count) puts("  No usable result files yet.\n");
        for (i = 0; i < catalog->count; ++i)
            printf("  %3u  %-32s  %u references\n\n", (unsigned)(i + 1),
                   catalog->files[i].name, (unsigned)catalog->files[i].references);
        if (catalog->invalid)
            printf("Skipped %u .txt files with invalid input.\n\n", (unsigned)catalog->invalid);
        if (catalog->omitted)
            printf("%u additional valid files are not shown; load those by name or full path.\n\n",
                   (unsigned)catalog->omitted);
    }
    puts("Enter a file number, a result file name, or a full path.");
    printf("0 or Enter: cancel\n> ");
    fflush(stdout);
    status = read_line(entry, sizeof(entry));
    if (!status) { free(catalog); return 0; }
    if (status < 0) { puts("Path is too long."); free(catalog); return 1; }
    remove_path_quotes(entry);
    if (!entry[0] || strcmp(entry, "0") == 0) { free(catalog); return 1; }
    if (numeric_selection(entry)) {
        if (!parse_integer(entry, 1, (int)catalog->count, &selection)) {
            puts("No file with this number. Previous references are unchanged.");
            free(catalog);
            return 1;
        }
        strcpy(entry, catalog->files[selection - 1].name);
    }
    free(catalog);
    if (resolve_reference_path(entry, directory, path, sizeof(path), error, sizeof(error)) &&
        load_references(path, references, error, sizeof(error)))
        printf("Loaded %u references from %s\n", (unsigned)references->count, path);
    else printf("Load failed: %s\nPrevious references are unchanged.\n", error);
    return 1;
}

static int save_to_results(const char *directory, const ReferenceString *references)
{
    char entry[RESULT_PATH_CAPACITY], path[RESULT_PATH_CAPACITY], error[512];
    int status;
    printf("\nDefault save directory:\n%s\n\n", directory);
    puts("Enter a name (e.g. experiment.txt) to save in results.");
    puts("Enter: choose the next unused result_001.txt-style name.");
    puts("A full or explicit relative path saves to that location.");
    printf("An existing file with the specified name will be replaced.\nSave as: ");
    fflush(stdout);
    status = read_line(entry, sizeof(entry));
    if (!status) return 0;
    if (status < 0) { puts("Path is too long."); return 1; }
    remove_path_quotes(entry);
    if (!strchr(entry, '/') && !strchr(entry, '\\') && !strchr(entry, ':') &&
        !ensure_results_directory(directory, error, sizeof(error))) {
        printf("Save failed: %s\n", error);
        return 1;
    }
    if ((!entry[0] ? next_result_path(directory, path, sizeof(path), error, sizeof(error))
                   : resolve_reference_path(entry, directory, path, sizeof(path), error, sizeof(error))) &&
        save_references(path, references, error, sizeof(error)))
        printf("Saved %u references to %s\n", (unsigned)references->count, path);
    else printf("Save failed: %s\n", error);
    return 1;
}

static int run_search(ReferenceString *references, int *frames)
{
    SearchConfig config;
    SearchResult result;
    int condition, limit;
    unsigned long total;
    puts("\nFind an interesting input\n\n"
         "1. FIFO has fewer faults than LRU\n"
         "2. LRU has fewer faults than FIFO\n"
         "3. FIFO has more faults with one extra frame\n\n"
         "Press Enter to use each default (a quick LRU-vs-FIFO example).\n");
    if (!ask_integer("Condition (1-3)", 1, 3, 2, &condition)) return 0;
    config.condition = (SearchCondition)condition;
    if (!ask_integer("Number of page IDs (1-6, starting at 0)", 1, SEARCH_MAX_PAGES, 3,
                     &config.page_count) ||
        !ask_integer("Exact reference length (1-12)", 1, SEARCH_MAX_LENGTH, 5,
                     &config.length) ||
        !ask_integer(config.condition == FIFO_BELADY
                     ? "Base frames (1-9, compare F with F+1)" : "Frames (1-10)",
                     1, config.condition == FIFO_BELADY ? MAX_FRAMES - 1 : MAX_FRAMES,
                     2, &config.frame_count) ||
        !ask_integer("Maximum candidates to test (1-100000)", 1,
                     (int)SEARCH_MAX_CANDIDATES, (int)SEARCH_MAX_CANDIDATES, &limit))
        return 0;
    config.limit = (unsigned long)limit;
    if (!search_candidate_count(&config, &total)) {
        puts("Invalid search configuration.");
        return 1;
    }
    printf("\nCondition: %s\n", search_condition_name(config.condition));
    printf("Page IDs: 0..%d    Exact length: %d    Frames: %d\n",
           config.page_count - 1, config.length, config.frame_count);
    printf("Candidates in this space: %lu    Search limit: %lu\n", total, config.limit);
    puts("Searching in lexicographic order; stopping at the first match...");
    fflush(stdout);
    search_references(&config, &result);
    printf("\nTested: %lu / %lu candidates\n", result.tested, result.total);
    if (result.status == SEARCH_FOUND) {
        puts("Match found.\n");
        show_references(&result.references);
        if (config.condition == FIFO_BELADY) {
            printf("FIFO with %d frames: %u faults\n", config.frame_count, (unsigned)result.left.faults);
            printf("FIFO with %d frames: %u faults\n", config.frame_count + 1, (unsigned)result.right.faults);
        } else {
            printf("FIFO: %u faults\nLRU:  %u faults\n",
                   (unsigned)result.left.faults, (unsigned)result.right.faults);
        }
        *references = result.references;
        *frames = config.frame_count;
        puts("\nThe found input and frame count are now active.");
        puts("Use option 10 to analyze FIFO/LRU, option 8 for frame counts, or option 11 to save.");
        puts("This is the first match in this search order, not a claim of shortest input.");
    } else if (result.status == SEARCH_EXHAUSTED) {
        puts("No match in the complete configured search space.");
        puts("This does not rule out other lengths, page counts, or frame counts.");
        puts("Previous input and frame count are unchanged.");
    } else if (result.status == SEARCH_LIMIT_REACHED) {
        puts("Search limit reached before checking every candidate. No match in the tested prefix.");
        puts("Untested candidates may still contain a match.");
        puts("Previous input and frame count are unchanged.");
    } else puts("Invalid search configuration.");
    return 1;
}

int main(int argc, char **argv)
{
    ReferenceString references = {{0}, 0};
    int frames = 3;
    Algorithm algorithm = FIFO;
    char line[RESULT_PATH_CAPACITY];
    char error[256];
    char results_directory[RESULT_PATH_CAPACITY] = "results";

    if (argc > 2) {
        puts("Usage: memory_policy_lab [reference-file.txt]");
        return 1;
    }
    if (!initialize_results(results_directory, sizeof(results_directory), error, sizeof(error)))
        printf("Results directory unavailable: %s\nFull paths can still be used.\n", error);
    if (argc == 2) {
        char path[RESULT_PATH_CAPACITY];
        if (!resolve_reference_path(argv[1], results_directory, path, sizeof(path), error, sizeof(error)) ||
            !load_references(path, &references, error, sizeof(error))) {
            fprintf(stderr, "%s\n", error);
            return 1;
        }
        printf("Loaded %u references.\n", (unsigned)references.count);
    }

    for (;;) {
        int choice;
        int status;
        printf("\nMemory Policy Lab\n\n");
        printf("Frames: %d    Algorithm: %s    References: %u\n\n",
               frames, algorithm_name(algorithm), (unsigned)references.count);
        puts("1. Load input (browse results or enter a path)\n"
             "2. Set number of frames\n"
             "3. Select algorithm\n"
             "4. Run step by step (fresh start)\n"
             "5. Run all (fresh start)\n"
             "6. Compare algorithms\n"
             "7. Show reference string\n"
             "8. Compare frame counts / Belady's anomaly\n"
             "9. Find an interesting input\n"
             "10. Analyze FIFO and LRU side by side\n"
             "11. Save input to results (or enter a full path)\n"
             "0. Exit");
        printf("Choose: ");
        fflush(stdout);
        status = read_line(line, sizeof(line));
        if (status == 0) break;
        if (status < 0 || !parse_integer(line, 0, 11, &choice)) {
            puts("Please enter a menu number from 0 to 11.");
            continue;
        }
        if (choice == 0) break;
        if (choice == 9) {
            if (!run_search(&references, &frames)) break;
        } else if (choice == 1) {
            if (!load_from_results(results_directory, &references)) break;
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
        } else if (choice == 10) {
            int mode;
            if (!ask_integer("Analysis mode (1 = step by step, 2 = show all)", 1, 2, 2, &mode))
                break;
            compare_traces(&references, frames, mode == 1);
        } else if (choice == 11) {
            if (!save_to_results(results_directory, &references)) break;
        }
    }
    puts("Goodbye.");
    return 0;
}
