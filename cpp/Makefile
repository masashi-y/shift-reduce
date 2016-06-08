CXX = g++
TARGET = parser
CXXFLAGS = -Wall -O3 -std=c++11
SRCS = main.cpp state.cpp tokens.cpp utils.cpp
OBJS := $(SRCS:.cpp=.o)

$(TARGET): $(OBJS)
	$(CXX) -o $@ $(OBJS)

clean:
	rm -f $(TARGET) $(OBJS)
