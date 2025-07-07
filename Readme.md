
Objective

This is a simplified shell implementation in C that replicates basic functionalities of a UNIX/Linux shell. It provides a command-line interface (CLI) to execute built-in and external commands, support piping, I/O redirection, and background process execution.

(System Calls )
A parent process will give a prompt shell> to the user and wait for a command (from a set of commands given below) to be typed. For each command, the parent process will fork a child process which will execute that command. Normally, the parent process will wait for the command to be finished, after which it will give the shell> prompt again and wait for the next command.
However, if the command is followed by an & (ampersand) character (with or without blanks after the command), the parent process will return immediately (that is, it will not wait for the child to finish the command), and wait for the next command.
Implemented the following commands:-
1. pwd - shows the present working directory
2. cd <directory_name> - changes the present working directory to the directory
specified in <directory_name>. A full path to a directory may be given.
3. mkdir <directory_name> - creates a new directory specified by <dir>.
4. ls <flag> - shows the contents of a directory based on some optional flags, like
ls -al, ls, etc.
5. exit - exits the shell
6. help - prints this list of commnds
7. If you type any other command at the prompt,the shell should just assume it is
an executable file and try to directly execute it.
Note:- A proper error message should be shown at the terminal/prompt, if the above
manual is not followed.

using I/O redirection to make it possible to:-
1. pipe one command through the other using the ‘|’ operator. For example, it should be possible to give the following commands to your program:
ls –al | more or cat text.txt | grep abcd

 Input Redirection (<)
	•	Redirects input to a command from a specified file.
	•	Example:cat < input.txt

 Output Redirection
	•	Overwrite Output (>): Writes the output of a command to a specified file, overwriting its contents.
	•	Example: echo "abc" > output.txt

    Append Output (>>): Appends the output of a command to the specified file without overwriting.
	•	Example: echo "Additional Line" >> output.txt


some shell functionalities:

1. Multi-Line commands:- A single command may be extended to a multiline command using backslash(\) character.
2. Command history:- Using the up arrow we can execute the previous commands.
3. Command Editing:- If a long command has some mistake, the user may press Ctrl+A to move the cursor to the start of the line and make corrections.


ps -axj

The ps -axj command shows all processes on the system (with or without controlling terminals) in job control format, which includes additional information like Process Group ID (PGID) and Session ID (SID).

Shell> top  
custom shell should fork a child, run top, and you should see the usual process monitoring screen (on macOS, you press Q to quit).

Shell> sleep 5   
The prompt will only reappear after 5 seconds. That confirms the parent truly waited for the child via waitpid().




Process Handling
	•	Utilizes fork() and execvp() for creating child processes and executing commands.
	•	Handles foreground and background processes effectively.

Error Handling
	•	Displays clear error messages for:
	•	Invalid commands.
	•	Invalid or inaccessible file paths.
	•	Incorrect usage of commands.
    If the file is missing or not executable, execvp() will print an error like Execvp() failed: No such file or directory.

 Segments for History Management
	1.	Using the GNU Readline Library
	•	The following headers are included at the top of your program:
       #include <readline/readline.h>
       #include <readline/history.h>
    History Navigation:
	•	After a command is added, the user can navigate the history using:
	•	Up Arrow: Displays the previous command.
	•	Down Arrow: Displays the next command.

	