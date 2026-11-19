CXX ?= g++
CXXFLAGS ?= -std=c++23 -Wall -Wextra -Wpedantic -g -O0

# Each practice file is a standalone program with its own main().
SRC ?= practice.cpp
BUILD_DIR := build
TARGET := $(BUILD_DIR)/$(basename $(notdir $(SRC)))

.PHONY: all run test clean help
.DEFAULT_GOAL := all

all: $(TARGET)

$(TARGET): $(SRC) Makefile | $(BUILD_DIR)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -MMD -MP $(SRC) -o $@ $(LDFLAGS) $(LDLIBS)

$(BUILD_DIR):
	mkdir -p $@

run: $(TARGET)
	./$(TARGET) $(ARGS)

# The assertions in the practice program serve as its tests.
test: run
	@echo "Tests passed."

clean:
	rm -rf $(BUILD_DIR)

help:
	@echo "make                         Build practice.cpp"
	@echo "make run                     Build and run"
	@echo "make test                    Run the assertion tests"
	@echo "make run SRC=example.cpp     Build and run another practice file"
	@echo "make run ARGS='arg1 arg2'    Pass arguments to the program"
	@echo "make clean                   Remove build output"

-include $(TARGET).d
