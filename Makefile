# Compiler
CXX = g++
CXXFLAGS = -std=c++17 -O2 -Wall

# Source files
SRC = main.cpp
OUT = game

# Libraries required for raylib on Linux
LIBS = -lraylib -lGL -lm -lpthread -ldl -lrt -lX11

all: $(OUT)

$(OUT): $(SRC)
	$(CXX) $(CXXFLAGS) $(SRC) -o $(OUT) $(LIBS)

run: $(OUT)
	./$(OUT)

clean:
	rm -f $(OUT)
