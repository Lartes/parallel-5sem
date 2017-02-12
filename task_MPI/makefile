MPI= mpicxx
CXX= g++
CXXFLAGS= -Wall -std=c++03

all: main simple

main: main.cpp
	$(MPI) main.cpp -o main

simple: simple.cpp
	$(CXX) $(CXXFLAGS) simple.cpp -o simple

clean: 
	rm main
	rm simple
