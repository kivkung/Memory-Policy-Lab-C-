CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -Wpedantic -O2

.PHONY: all test

all: build/memory_policy_lab

build:
	mkdir -p build

build/memory_policy_lab: src/main.c src/simulator.c src/input.c src/display.c src/search.c src/results.c src/simulator.h src/input.h src/display.h src/search.h src/results.h | build
	$(CC) $(CFLAGS) src/main.c src/simulator.c src/input.c src/display.c src/search.c src/results.c -o $@

build/test_simulator: tests/test_simulator.c src/simulator.c src/input.c src/simulator.h src/input.h | build
	$(CC) $(CFLAGS) -Isrc tests/test_simulator.c src/simulator.c src/input.c -o $@

build/test_search: tests/test_search.c src/search.c src/simulator.c src/input.c src/search.h src/simulator.h src/input.h | build
	$(CC) $(CFLAGS) -Isrc tests/test_search.c src/search.c src/simulator.c src/input.c -o $@

build/test_results: tests/test_results.c src/results.c src/input.c src/results.h src/input.h src/simulator.h | build
	$(CC) $(CFLAGS) -Isrc tests/test_results.c src/results.c src/input.c -o $@

test: build/test_simulator build/test_search build/test_results
	./build/test_simulator
	./build/test_search
	./build/test_results
