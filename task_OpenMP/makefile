CXX= g++
CXXFLAGS= -Wall -std=c++03

all: gen main simple

gen: gen.cpp
	$(CXX) $(CXXFLAGS) gen.cpp -o gen

main: main.cpp
	$(CXX) $(CXXFLAGS) -fopenmp main.cpp -o main

simple: simple.cpp
	$(CXX) $(CXXFLAGS) simple.cpp -o simple

clean: 
	rm gen
	rm main
	rm simple
