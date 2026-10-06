#include "results.h"
#include "input.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>
#ifdef _WIN32
#include <direct.h>
#define REMOVE_DIRECTORY _rmdir
#else
#include <unistd.h>
#define REMOVE_DIRECTORY rmdir
#endif

int main(void)
{
    const char *directory = "build/result-test-fixtures";
    char path[RESULT_PATH_CAPACITY], error[512], initialized[RESULT_PATH_CAPACITY];
    ReferenceString references = {{0, 1, 0, 2, 0}, 5};
    ResultCatalog catalog;
    FILE *file;

    assert(initialize_results(initialized, sizeof(initialized), error, sizeof(error)));
    assert(strstr(initialized, "results"));
    assert(ensure_results_directory(directory, error, sizeof(error)));
    assert(ensure_results_directory(directory, error, sizeof(error)));
    assert(list_result_files(directory, &catalog, error, sizeof(error)));
    assert(catalog.count == 0);
    assert(resolve_reference_path("a", directory, path, sizeof(path), error, sizeof(error)));
    assert(strstr(path, "a.txt"));
    assert(save_references(path, &references, error, sizeof(error)));
    assert(resolve_reference_path("B.TXT", directory, path, sizeof(path), error, sizeof(error)));
    assert(strstr(path, "B.TXT") && !strstr(path, ".TXT.txt"));
    assert(save_references(path, &references, error, sizeof(error)));
    assert(resolve_reference_path("C:\\some folder\\input.txt", directory, path, sizeof(path), error, sizeof(error)));
    assert(strcmp(path, "C:\\some folder\\input.txt") == 0);
    assert(resolve_reference_path("/tmp/input.txt", directory, path, sizeof(path), error, sizeof(error)));
    assert(strcmp(path, "/tmp/input.txt") == 0);
    assert(resolve_reference_path("examples/classic.txt", directory, path, sizeof(path), error, sizeof(error)));
    assert(strcmp(path, "examples/classic.txt") == 0);
    assert(!resolve_reference_path("", directory, path, sizeof(path), error, sizeof(error)));
    assert(!resolve_reference_path("a", directory, path, 2, error, sizeof(error)));

    assert(next_result_path(directory, path, sizeof(path), error, sizeof(error)));
    assert(strstr(path, "result_001.txt"));
    assert(save_references(path, &references, error, sizeof(error)));
    assert(next_result_path(directory, path, sizeof(path), error, sizeof(error)));
    assert(strstr(path, "result_002.txt"));

    file = fopen("build/result-test-fixtures/invalid.txt", "wb");
    assert(file && fputs("not a reference string", file) >= 0 && fclose(file) == 0);
    file = fopen("build/result-test-fixtures/ignored.md", "wb");
    assert(file && fputs("1 2 3", file) >= 0 && fclose(file) == 0);
    assert(ensure_results_directory("build/result-test-fixtures/folder.txt", error, sizeof(error)));
    assert(list_result_files(directory, &catalog, error, sizeof(error)));
    assert(catalog.count == 3 && catalog.invalid == 1 && catalog.omitted == 0);
    assert(strcmp(catalog.files[0].name, "a.txt") == 0);
    assert(strcmp(catalog.files[1].name, "B.TXT") == 0);
    assert(strcmp(catalog.files[2].name, "result_001.txt") == 0);
    assert(catalog.files[0].references == 5);
    assert(!ensure_results_directory("build/result-test-fixtures/a.txt", error, sizeof(error)));

    assert(remove("build/result-test-fixtures/a.txt") == 0);
    assert(remove("build/result-test-fixtures/B.TXT") == 0);
    assert(remove("build/result-test-fixtures/result_001.txt") == 0);
    assert(remove("build/result-test-fixtures/invalid.txt") == 0);
    assert(remove("build/result-test-fixtures/ignored.md") == 0);
    assert(REMOVE_DIRECTORY("build/result-test-fixtures/folder.txt") == 0);
    assert(REMOVE_DIRECTORY(directory) == 0);
    puts("Results tests passed: directory creation, path resolution, usable-file listing, sorting, automatic names.");
    return 0;
}
