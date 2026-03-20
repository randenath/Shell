# CS441/541 Shell Project

## Author(s):
Rohan Hari


## Date:
Last Modified: 03/10/2026


## Description:
This is the shell project (project 3). In this project, we make our own shell program.


## How to build the software
With the given make file, you are able to just type in "make" to compile the program and make an executable called "mysh".


## How to use the software
There are two modes that the program can run in: Interactive and Batch.

To enter Interactive mode, do not type any command line inputs when you execute the executable file. Then you will see text in the terminal that says "Interactive Mode!" Then you are able to enter any commands you want. Have fun!

To enter Batch mode, enter a filepath when you execute the executable file. Then you will see text in the terminal that says "Batch Mode!" This will run the commands in the text file. Almost as fun as Interactive mode!


## How the software was tested
The main way that I tesed was to run the "make check" as well as making my own 5 test files.

5 Test Files:

- test1.txt : Basic commands. Easy Peasy

- test2.txt : Background tests using &, jobs, and wait

- test3.txt : File redirection

- test4.txt : fg tests (both no arguments and with ID)

- test5.txt : Combined tests. We run the program at MAXIMUM POWER here. Careful of overloading your computer.


## Known bugs and problem areas
Only bug I had was the exit issue. But that has now been fixed and the shell is ready for production.