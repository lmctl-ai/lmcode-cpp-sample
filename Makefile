VERSION ?= 0.4.0
CXX ?= c++
CXXFLAGS ?= -std=c++17 -O2 -Wall -Wextra -Werror -pedantic
BUILD ?= build
LMSCRIPT_BIN ?= lmscript
SDK ?= vendor/lmcode
SUITE ?= sample-suite
INCLUDES = -Isrc -I"$(BUILD)" -I"$(SDK)/include" -I"$(SDK)/stdlib"
HEADERS = $(wildcard $(SDK)/include/*.hpp $(SDK)/stdlib/*.hpp)
INCLUDES += '-DSAMPLE_VERSION="$(VERSION)"'
.PHONY: all test test-dry-adoption clean
all: $(BUILD)/lmcode-cpp-sample
$(BUILD)/sample_schema.hpp: $(SUITE)/sample.dsl.json scripts/embed-schema.py
	mkdir -p $(BUILD)
	python3 scripts/embed-schema.py $< $@
$(BUILD)/lmcode-cpp-sample: src/main.cpp src/shop.cpp src/conversions.cpp src/shop.hpp $(HEADERS) $(BUILD)/sample_schema.hpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) src/main.cpp src/shop.cpp src/conversions.cpp -o "$@"
$(BUILD)/dry-adoption-test: tests/dry-adoption.cpp src/shop.cpp src/conversions.cpp src/shop.hpp $(HEADERS) $(BUILD)/sample_schema.hpp
	mkdir -p $(BUILD)
	$(CXX) $(CXXFLAGS) -DH6_TEST_COUNTERS $(INCLUDES) tests/dry-adoption.cpp src/shop.cpp src/conversions.cpp -o "$@"
test-dry-adoption: $(BUILD)/dry-adoption-test $(BUILD)/lmcode-cpp-sample
	$(BUILD)/dry-adoption-test "$(abspath $(SUITE)/bytecode/024-file-functions.dslb)"
	python3 tests/check-dry-adoption.py "$(abspath $(BUILD)/lmcode-cpp-sample)"
test: all test-dry-adoption
	python3 scripts/sync.py check
	LMCODE_SUITE="$(abspath $(SUITE))" $(LMSCRIPT_BIN) "$(SUITE)/audit.lms" --std "$(SDK)/std/lmcode-std.schema.json"
	bash "$(SUITE)/run-suite.sh" "$(abspath $(BUILD)/lmcode-cpp-sample)"
	python3 sample-suite/check-invocations.py "$(abspath $(BUILD)/lmcode-cpp-sample)"
clean:
	rm -rf $(BUILD)

ifeq ($(OS),Windows_NT)
CXXFLAGS += -municode
endif
