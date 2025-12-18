# The compiler we are using
CC=gcc
# Compiler flags: -Wall (show all warnings), -g (add debug info)
CFLAGS=-Wall -g

# The name of our final program
TARGET=shell.out

# All the .c source files in the 'src' directory
SOURCES=$(wildcard src/*.c)
# The corresponding object files (e.g., src/main.c -> src/main.o)
OBJECTS=$(SOURCES:.c=.o)

# The default command to run when you just type "make"
all: $(TARGET)

# This rule tells make how to build the final executable (shell.out)
# It depends on all the object files (.o)
$(TARGET): $(OBJECTS)
	# This is the "linking" step. It combines all .o files into one program.
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJECTS)

# This is a generic rule that tells make how to build a .o file from a .c file
# It depends on the .c file and its corresponding header file in 'include'
src/%.o: src/%.c include/shell.h
	# This is the "compiling" step. It turns a single .c file into machine code.
	$(CC) $(CFLAGS) -Iinclude -c $< -o $@

# A command to clean up all the compiled files
clean:
	rm -f src/*.o $(TARGET)

