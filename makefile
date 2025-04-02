CXX = g++
# Use -std=c++11 or newer
CXXFLAGS = -std=c++11 -Wall -Wextra -O2
TARGET = pathfinder
# List all your .cpp files here
SOURCES = main.cpp hexpathfinder_draw.cpp
OBJECTS = $(SOURCES:.cpp=.o)
HEADERS = hexpathfinder.h

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJECTS)

%.o: %.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(TARGET) $(OBJECTS) maze.ps
