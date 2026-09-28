CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -Wpedantic -O2

.PHONY: all test

all: build/simulator

build:
	mkdir -p build

build/simulator: src/main.c src/simulator.c src/input.c src/display.c src/simulator.h src/input.h src/display.h | build
	$(CC) $(CFLAGS) src/main.c src/simulator.c src/input.c src/display.c -o $@

build/test_simulator: tests/test_simulator.c src/simulator.c src/input.c src/simulator.h src/input.h | build
	$(CC) $(CFLAGS) -Isrc tests/test_simulator.c src/simulator.c src/input.c -o $@

test: build/test_simulator
	./build/test_simulator
