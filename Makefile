# SW-QPS Network Simulator Makefile

CXX = g++
CXXFLAGS = -std=c++17 -O3 -Wall -Wextra -march=native -pthread
DEBUG_FLAGS = -g -O0 -DDEBUG

# Source files
SOURCES = sw_qps_simulator.cpp
HEADERS = sw_qps_simulator.h
OBJECTS = $(SOURCES:.cpp=.o)

# Executable name
TARGET = sw_qps_sim
DEBUG_TARGET = sw_qps_sim_debug

# Default target
all: $(TARGET)

# Release build
$(TARGET): $(SOURCES) $(HEADERS)
	$(CXX) $(CXXFLAGS) $(SOURCES) -o $(TARGET)

# Debug build
debug: $(DEBUG_TARGET)

$(DEBUG_TARGET): $(SOURCES) $(HEADERS)
	$(CXX) $(CXXFLAGS) $(DEBUG_FLAGS) $(SOURCES) -o $(DEBUG_TARGET)

# Clean
clean:
	rm -f $(TARGET) $(DEBUG_TARGET) $(OBJECTS) *.csv

# Run with default parameters
run: $(TARGET)
	./$(TARGET)

# Run with custom parameters
test: $(TARGET)
	@echo "Running quick test with reduced parameters..."
	./$(TARGET) test

# Generate performance plots (requires Python)
plot: run
	python3 plot_results.py sw_qps_results.csv

# Help
help:
	@echo "SW-QPS Network Simulator Makefile"
	@echo ""
	@echo "Usage:"
	@echo "  make          - Build release version"
	@echo "  make debug    - Build debug version"
	@echo "  make run      - Build and run with default parameters"
	@echo "  make test     - Build and run quick test"
	@echo "  make clean    - Remove built files and results"
	@echo "  make plot     - Generate performance plots (requires Python)"
	@echo "  make help     - Show this help message"

.PHONY: all clean run test debug plot help
