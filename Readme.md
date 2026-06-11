# Linux Shell in C

![Language](https://img.shields.io/badge/Language-C-blue) ![Platform](https://img.shields.io/badge/Platform-Linux%20%7C%20macOS-lightgrey) ![License](https://img.shields.io/badge/License-MIT-green)

## Objective

This is a simplified shell implementation in C that replicates basic functionalities of a UNIX/Linux shell. It provides a command-line interface (CLI) to execute built-in and external commands, support piping, I/O redirection, background process execution, alias management, and command history.

## How It Works (System Calls)

A parent process gives a prompt to the user and waits for a command. For each command, the parent process forks a child process which executes that command. Normally, the parent process waits for the command to finish, after which it gives the prompt again.

However, if the command is followed by an `&` (ampersand) character, the parent process returns immediately without waiting for the child to finish, and waits for the next command.

## Built-in Commands

| Command | Description |
|---|---|
| `pwd` | Shows the present working directory |
| `cd <dir>` | Changes the present working directory (bare `cd` goes home) |
| `mkdir <dir>` | Creates a new directory |
| `ls` | Shows directory contents (supports flags like `ls -al`) |
| `exit` | Exits the shell |
| `help` | Prints the list of available commands |
| `alias name='value'` | Creates a command shortcut |
| `unalias name` | Removes an alias |
| `jobs` | Lists active background jobs |
| `history` | Prints full command history |
| `echo $VAR` | Prints value of environment variable |

## Features

### Piping (`|`)
Chain commands using the pipe operator:
```
Shell$ ls -al | grep .c
Shell$ cat input.txt | grep hello
```

### Input Redirection (`<`)
Redirect input to a command from a file:
```
Shell$ cat < input.txt
```

### Output Redirection (`>`)
Write output to a file, overwriting contents:
```
Shell$ echo "hello" > output.txt
```

### Append Output (`>>`)
Append output to a file without overwriting:
```
Shell$ echo "Additional Line" >> output.txt
```

### Background Execution (`&`)
Run a command in the background:
```
Shell$ sleep 10 &
[1] 12345
Shell$ jobs
[1] Running   PID 12345   sleep
```

### Alias Support
Create shortcuts for long commands:
```
Shell$ alias ll='ls -al'
Shell$ ll
Shell$ alias gs='git status'
Shell$ unalias ll
```

### Echo with Environment Variable Expansion
```
Shell$ echo $HOME
/Users/parulahlawat
Shell$ echo $PATH
Shell$ echo $USER
```

### Signal Handling (Ctrl+C)
Pressing `Ctrl+C` cancels the current running command but does **not** exit the shell — just like bash behavior.

### Multi-Line Commands
A single command may be extended to multiple lines using the backslash `\` character:
```
Shell$ echo "line one" \
> "line two"
```

### Command History
- Use the **up/down arrow keys** to navigate previous commands
- Type `history` to print the full numbered list
- Press `Ctrl+A` to move cursor to start of line for editing

### Color Prompt
The shell displays a colorized prompt showing username, hostname, and current directory:
```
parulahlawat@Mac:~/Desktop/Linux_Shell$
```

### Restricted Words
Output redirection is blocked for commands containing `kill` or `death` to prevent accidental destructive writes.

## Build & Run

### Prerequisites
```bash
# macOS
xcode-select --install   # installs gcc

# Ubuntu/Debian
sudo apt install libreadline-dev gcc
```

### Compile
```bash
gcc -o shell shell.c -lreadline
```

### Run
```bash
./shell
```

## Process Handling

- Uses `fork()` and `execvp()` for creating child processes and executing commands
- Handles foreground and background processes effectively
- Background jobs are tracked and reported when they finish

## Error Handling

- Invalid commands: clear error message shown
- Invalid or inaccessible file paths: reported via `perror()`
- Incorrect usage: usage hints printed to stderr

## Testing Examples

```bash
# Basic
Shell$ pwd
Shell$ ls -al
Shell$ cd /tmp

# Pipe
Shell$ ls | grep shell

# Redirection
Shell$ echo "hello world" > test.txt
Shell$ cat < test.txt
Shell$ echo "line 2" >> test.txt

# Alias
Shell$ alias ll='ls -al'
Shell$ ll
Shell$ unalias ll

# Background
Shell$ sleep 30 &
Shell$ jobs

# History
Shell$ history

# Environment variables
Shell$ echo $HOME
Shell$ echo $USER

# Ctrl+C test
Shell$ sleep 10     # press Ctrl+C — shell stays alive
```

## Project Structure

```
Linux_Shell/
├── shell.c         # Main shell source code
├── fact.c          # Example: factorial program (used with shell)
├── Makefile        # Build configuration
├── input.txt       # Sample input for redirection demo
├── output.txt      # Sample output for redirection demo
└── Readme.md       # This file
```

## System Calls Used

| Call | Purpose |
|---|---|
| `fork()` | Create child processes |
| `execvp()` | Execute commands |
| `waitpid()` | Wait for foreground / reap background children |
| `pipe()` | Inter-process communication |
| `dup2()` | Redirect file descriptors |
| `open()` / `creat()` / `close()` | File operations |
| `chdir()` / `getcwd()` | Directory navigation |
| `signal()` | Handle Ctrl+C gracefully |
| `readline()` / `add_history()` | Input with history (GNU Readline) |
