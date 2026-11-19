CXX ?= g++
CXXFLAGS ?= -std=c++23 -Wall -Wextra -Wpedantic -g -O0

# The variant implementation has no main(), so compile it as an object file.
SRC ?= variant.cpp
BUILD_DIR := build
TARGET := $(BUILD_DIR)/$(basename $(notdir $(SRC))).o

TEST_SRC := test.cpp
TEST_TARGET := $(BUILD_DIR)/test

.PHONY: all run test test-compile-fail clean help
.DEFAULT_GOAL := all

all: $(TARGET)

$(TARGET): $(SRC) Makefile | $(BUILD_DIR)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -MMD -MP -c $(SRC) -o $@

$(TEST_TARGET): $(TEST_SRC) variant.cpp Makefile | $(BUILD_DIR)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -UNDEBUG -MMD -MP $(TEST_SRC) -o $@ $(LDFLAGS) $(LDLIBS)

$(BUILD_DIR):
	mkdir -p $@

run: $(PRACTICE_TARGET)
	./$(PRACTICE_TARGET) $(ARGS)

# Compile-time rejection checks use the same test file with invalid API calls enabled.
test-compile-fail: $(TEST_TARGET)
	@set -e; for scenario in DUPLICATE_TYPE MISSING_TYPE INVALID_INDEX; do \
		if $(CXX) $(CPPFLAGS) $(CXXFLAGS) -UNDEBUG -DTEST_$$scenario -fsyntax-only $(TEST_SRC) >$(BUILD_DIR)/test-$$scenario.log 2>&1; then \
			echo "Expected compilation failure for $$scenario"; exit 1; \
		fi; \
	done
	@echo "Compile-time rejection tests passed."

test: $(TEST_TARGET) test-compile-fail
	./$(TEST_TARGET)

clean:
	rm -rf $(BUILD_DIR)

help:
	@echo "make                                  Compile variant.cpp"
	@echo "make run                              Build and run tmp/practice.cpp"
	@echo "make test                             Build and run test.cpp and rejection checks"
	@echo "make run PRACTICE_SRC=example.cpp      Build and run another practice file"
	@echo "make run ARGS='arg1 arg2'             Pass arguments to the program"
	@echo "make clean                            Remove build output"

-include $(TARGET:.o=.d) $(PRACTICE_TARGET).d $(TEST_TARGET).d
