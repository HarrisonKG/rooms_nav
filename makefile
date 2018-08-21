######################################################################
# Program name: Program 2
# Author: Kristen Harrison
# Description: This makefile can be run with three commands:
# 1. "make rooms" makes the program rooms and "make prog2" runs the interactive portion 
# 2. "make clean" cleans the directory   
# 3. "make memcheckrooms" or "make memcheckprog2" runs valgrind to test for memory leaks
######################################################################

# target: dependencies
# [tab] recipe


CXX = gcc
CXXFLAGS = -g -Wall -std=c89 -lpthread -pthread
VALFLAGS = --leak-check=yes --show-reachable=yes

# full valgrind flags: --tool=memcheck --leak-check=full --track-origins=yes --show-leak-kinds=all


rooms: harrisk4.buildrooms.o 
	$(CXX) harrisk4.buildrooms.o -o rooms

prog2: harrisk4.adventure.o 
	$(CXX) $(CXXFLAGS) harrisk4.adventure.o -o prog2

harrisk4.buildrooms.o: harrisk4.buildrooms.c
	$(CXX) $(CXXFLAGS) -c harrisk4.buildrooms.c

harrisk4.adventure.o: harrisk4.adventure.c
	$(CXX) $(CXXFLAGS) -c harrisk4.adventure.c


clean:
	rm -rf *.o rooms prog2 harrisk4.rooms.*

memcheckrooms: rooms
	valgrind $(VALFLAGS) ./rooms

memcheckprog2: prog2
	valgrind $(VALFLAGS) ./prog2

