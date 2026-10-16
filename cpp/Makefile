# ============================================================
# Makefile for Banking Anomaly Detector (PBL)
# ============================================================

CXX      := g++
CXXFLAGS := -std=c++17 -Wall -Wextra -O2 -Iinclude
LDFLAGS  :=

SRC := src/main.cpp \
       src/rules.cpp \
       src/graph.cpp \
       src/scorer.cpp \
       src/generator.cpp \
       src/io.cpp

OBJ := $(SRC:.cpp=.o)
TARGET := detector

.PHONY: all clean run

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)
	@echo ""
	@echo "Build successful → ./detector"
	@echo "Run with:  ./detector"

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) $(TARGET)
	rm -f data/*.csv

run: $(TARGET)
	./detector
