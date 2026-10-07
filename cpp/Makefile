CXX      ?= g++
CXXFLAGS ?= -std=c++17 -O2 -Wall -Wextra -Iinclude
LIB_SRC  := src/rules.cpp src/graph.cpp src/ml.cpp src/scorer.cpp src/generator.cpp src/io.cpp src/pipeline.cpp

all: detector

detector: $(LIB_SRC) src/main.cpp
	$(CXX) $(CXXFLAGS) $^ -o $@

test_core: $(LIB_SRC) tests/test_core.cpp
	$(CXX) $(CXXFLAGS) $^ -o $@

test: test_core
	./test_core

run: detector
	./detector

clean:
	rm -f detector test_core detector.exe test_core.exe
