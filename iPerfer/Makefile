## CSCI 4430 Advanced Makefile

# How to use this Makefile...
###################
###################
##               ##
##  $ make help  ##
##               ##
###################
###################

# TODO Change to g++ for C++ only
CXX = g++
# TODO For C++ only.
# CXXFLAGS = -Wconversion -Wall -Werror -Wextra -pedantic 

all: iPerfer

# TODO Modify source file name for your project.
# For C only.
# For C++ only.
SOURCES = iPerfer.cpp
iPerfer: $(SOURCES)
	$(CXX) $(CXXFLAGS) $(SOURCES) -o iPerfer

clean:
	rm -rf iPerfer *.dSYM

.PHONY: clean
