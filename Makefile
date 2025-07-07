# Makefile

CC      = gcc
CFLAGS  = -Wall -Wextra -g
# For macOS, you typically need -lreadline and -lncurses:
LDFLAGS = -lreadline -lncurses

# If Homebrew installed readline in a non-standard path, uncomment and adjust:
# CFLAGS  += -I/usr/local/opt/readline/include
# LDFLAGS += -L/usr/local/opt/readline/lib

# The default make target:
all: shell

# Compile and link our shell
shell: shell.o
	$(CC) $(CFLAGS) shell.o -o shell $(LDFLAGS)

# Build the object file
shell.o: shell.c
	$(CC) $(CFLAGS) -c shell.c

# Clean up
clean:
	rm -f shell shell.o