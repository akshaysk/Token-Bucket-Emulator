***********************************************************
	Name: Akshay Sanjay Kale
	Course: Operating Systems (CS 402) AM Section
	Warm Up Assignment No:2
***********************************************************	

This file describes how to make use of the warmup2 assignment code you have at your disposal.

1]Source code Files:
1. Header Files:
	1.1 cs402.h
	1.2 my402list.h	
	1.3 warmup2.h

2. C files:
	2.1 my402list.c
	2.2 warmup2.c

3. Makefile


2]Instructions to Compile code:
1. make warmup2
	Creates warmup2 executable

2. make clean
	Deletes all temporary object files and executables


3]Description of source files

1. warmup2.h
	Contains definition of packet structure, a structure that will store all the command-line-input parameters and a set of initialized global variables which are needed in in statistics calculation.

2. warmup2.c
	Includes behavioural implementation of arrival thread, token thread and server thread. Also includes the statistics calculation logic.

3. cs402.h, my402list.h and my402list.c
	Includes doubly circular linked list implementation logic from previous warmup1 assignment.
	 
	

