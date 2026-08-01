CXX = g++
CXXFLAGS = -Wall -Wconversion -std=c++23

TARGET = main

SRCS = $(wildcard ./src/*.cpp)

OBJS = $(patsubst ./src/%.cpp, %.o, $(SRCS))

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) $^ -o $@

%.o: ./src/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f *.o $(TARGET)
