# Compiler to use
CC=gcc

# Compiler flags
CFLAGS=-Wall -Wextra -pthread

# Directory for object files
OBJDIR=obj

# Directory for binary files
BINDIR=bin

# Source files
SRCS=$(wildcard *.c)

# Object files
OBJS=$(patsubst %.c,$(OBJDIR)/%.o,$(SRCS))

# Executables
EXES=$(patsubst %.c,$(BINDIR)/%,$(SRCS))

# Target to build all executables
all: $(EXES)

# Rule to build object files
$(OBJDIR)/%.o: %.c | $(OBJDIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Rule to link executables
$(BINDIR)/%: $(OBJDIR)/%.o $(OBJDIR)/calcul.o | $(BINDIR)
	$(CC) $(CFLAGS) $^ -o $@

# Rule to create object directory
$(OBJDIR):
	mkdir $(OBJDIR)

# Rule to create binary directory
$(BINDIR):
	mkdir $(BINDIR)

# Rule to clean up object and binary files
clean:
	rm -rf $(BINDIR)

.PHONY: all clean

