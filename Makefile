CXXFLAGS:=-std=c++17 -O2 -Wall -Wextra $(shell libpng-config --cppflags)
LDFLAGS:=$(shell libpng-config --ldflags)
TARGET:=gpvrc-asm
SRCS:=main.cpp
OBJS:=$(SRCS:.cpp=.o)

$(TARGET): $(OBJS)
	$(CXX) -o $@ $^ $(CXXFLAGS) $(LDFLAGS)

%.o: %.cpp
	$(CXX) -o $@ -c $< $(CXXFLAGS)
