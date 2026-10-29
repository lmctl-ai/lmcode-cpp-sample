VERSION ?= 0.4.0
CXX ?= c++
CXXFLAGS ?= -std=c++17 -O2 -Wall -Wextra -Werror -pedantic
BUILD ?= build
SDK ?= vendor/lmcode
SUITE ?= sample-suite
INCLUDES = -Isrc -I"$(BUILD)" -I"$(SDK)/include" -I"$(SDK)/stdlib"
HEADERS = $(wildcard $(SDK)/include/*.hpp $(SDK)/stdlib/*.hpp)
INCLUDES += '-DSAMPLE_VERSION="$(VERSION)"'
.PHONY: all test clean
all: $(BUILD)/lmcode-cpp-sample
$(BUILD)/sample_schema.hpp: $(SUITE)/sample.dsl.json scripts/embed-schema.py
	mkdir -p $(BUILD)
	python3 scripts/embed-schema.py $< $@
$(BUILD)/lmcode-cpp-sample: src/main.cpp src/shop.cpp src/conversions.cpp src/shop.hpp $(HEADERS) $(BUILD)/sample_schema.hpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) src/main.cpp src/shop.cpp src/conversions.cpp -o "$@"
test: all
	python3 -B "$(SUITE)/audit.py" --std "$(SDK)/std/lmcode-std.schema.json"
	bash "$(SUITE)/run-suite.sh" "$(abspath $(BUILD)/lmcode-cpp-sample)"
clean:
	rm -rf $(BUILD)
