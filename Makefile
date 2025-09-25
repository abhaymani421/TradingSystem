# Makefile for Trading System

# Compiler
CXX = g++
CXXFLAGS = -std=c++20

# Source and output
SRCS = $(wildcard src/*.cpp)
OUT = trading_app.exe

# Default target
all: $(OUT)

# Compile
$(OUT): $(SRCS)
	$(CXX) $(CXXFLAGS) $(SRCS) -o $(OUT)

# Clean build files
clean:
	del $(OUT)  # On Windows
	# rm -f $(OUT)  # Use this line on Linux/Mac

.PHONY: all clean
