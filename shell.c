#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <sys/wait.h>
#include <fcntl.h>  // for creat(), open()

// Utility max() function
int max(int a, int b) {
    return (a > b) ? a : b;
}

// Execute an external command using execvp
// If execvp returns -1, print error and exit
void execute_command(char* args[], int n) {
    int execvp_code = execvp(args[0], args);
    if(execvp_code == -1) {
        perror("Execvp() failed");
        exit(EXIT_FAILURE);
    }
}


// Counts the number of space-delimited words in a line.
// Returns the word count as an integer.

int count_words(char* line) {
    int word_count = 0;
    char *token = strtok(line, " ");
    while(token != NULL) {
        token = strtok(NULL, " ");
        word_count++;
    }
    return word_count;
}

// Main function implementing a mini-shell with support for built-in commands,
// piped commands, background processes, and I/O redirection.
int main() {
    using_history(); // Initialize History for readline

    while (1) {
        char piped_command[1024] = "";
        short isAmps = 0; // Flag for background process

        // Read input (support multi-line with backslash)
        strcat(piped_command, readline("\nShell>"));
        while (piped_command[strlen(piped_command)-1] == '\\') {
            piped_command[strlen(piped_command)-1] = '\0';
            strcat(piped_command, readline("\n>"));
        }

        // If user simply presses enter or the command is empty, continue
        if(!strlen(piped_command))
            continue;
        add_history(piped_command);

        // Split the command by '|' to handle potential pipes
        char* piped_comm_args[100];
        int piped_arg_count = 0;
        char *Ptoken = strtok(piped_command, "|");
        while(Ptoken != NULL) {
            piped_comm_args[piped_arg_count] = Ptoken;
            Ptoken = strtok(NULL, "|");
            piped_arg_count++;
        }

        int file_des[2];    // Used for piping
        int inp_init = 0;   // Used to track input file descriptor across multiple pipes

        // Process each command segment in the pipeline
        for(int k = 0; k < piped_arg_count; k++) {
            char command[1024] = "";
            strcat(command, piped_comm_args[k]);
            if(!strlen(command))
                continue;

            // Tokenize by spaces
            char* comm_args[100];
            int arg_count = 0;
            char *token = strtok(command, " ");
            while(token != NULL) {
                comm_args[arg_count] = token;
                token = strtok(NULL, " ");
                arg_count++;
            }

            // Check if the last token ends with '&' => background process
            if(comm_args[arg_count-1] &&
               comm_args[arg_count-1][strlen(comm_args[arg_count-1]) - 1] == '&')
            {
                isAmps = 1;
                // If arg is exactly "&", remove it entirely
                if(strlen(comm_args[arg_count-1]) == 1)
                    arg_count--;
                else
                    // Otherwise just strip the '&' char
                    comm_args[arg_count-1][strlen(comm_args[arg_count-1]) - 1] = '\0';
            }
            // NULL-terminate argument array (for execvp usage)
            comm_args[arg_count] = NULL;
            arg_count++;

            // Create a pipe if this is not the last command
            if(k < piped_arg_count - 1) {
                if(pipe(file_des) == -1) {
                    perror("Pipe Error");
                    exit(EXIT_FAILURE);
                }
            }

            // Built-in commands
            if(strcmp(comm_args[0], "exit") == 0) {
                // Exit the shell
                exit(0);
            }
            if(strcmp(comm_args[0], "help") == 0) {
                // Display help
                printf("\nCommands Available:\n");
                printf("\t1. pwd\n");
                printf("\t2. cd <directory>\n");
                printf("\t3. mkdir <directory>\n");
                printf("\t4. ls <flag>\n");
                printf("\t5. exit\n");
                printf("\t6. help\n");
                printf("\t...any other external command\n");
                continue;
            }
            if(strcmp(comm_args[0], "cd") == 0) {
                // Change directory command
                char s1[100], s2[100], temp[1024] = "";
                getcwd(s1, 100);

                // Reconstruct directory path (in case it has spaces)
                strcat(temp, comm_args[1]);
                for(int i = 2; i < arg_count - 1; i++) {
                    strcat(temp, " ");
                    strcat(temp, comm_args[i]);
                }

                // Perform directory change
                if(chdir(temp) == 0) {
                    printf("\nPWD changed from: %s\n", s1);
                    printf("To: %s\n", getcwd(s2, 100));
                } else {
                    perror("cd command failed");
                }
                continue;
            }

            // Fork for external commands (or further piped segments)
            pid_t comm_child_pid = fork();
            if(comm_child_pid == -1) {
                perror("Fork() failed");
                exit(1);
            }
            else if(comm_child_pid == 0) {
                // CHILD PROCESS
                printf("\nInside Child Process with PID: %d\n", getpid());
                printf("Executing Command: %s\n\n", comm_args[0]);

                // If not the first command in pipeline, redirect input
                if(k > 0) {
                    dup2(inp_init, STDIN_FILENO);
                    close(inp_init);
                }
                // If not the last command, redirect output to pipe
                if(k < piped_arg_count - 1) {
                    dup2(file_des[1], STDOUT_FILENO);
                    close(file_des[0]);
                    close(file_des[1]);
                }

                // --- I/O Redirection ---
                {
                    int in_redir_index  = -1;
                    int out_redir_index = -1;
                    int append_mode     = 0;

                    // Check each arg for <, >, >>
                    for(int j = 0; j < arg_count - 1; j++) {
                        if(comm_args[j] && strcmp(comm_args[j], "<") == 0) {
                            in_redir_index = j;
                        }
                        else if(comm_args[j] && strcmp(comm_args[j], ">>") == 0) {
                            out_redir_index = j;
                            append_mode = 1;  // Mark append mode for '>>'
                        }
                        else if(comm_args[j] && strcmp(comm_args[j], ">") == 0) {
                            out_redir_index = j;
                            append_mode = 0;  // Overwrite mode for '>'
                        }
                    }

                    // Handle input redirection
                    if(in_redir_index != -1) {
                        char *infile = comm_args[in_redir_index + 1];
                        int fd_in = open(infile, O_RDONLY);
                        if(fd_in < 0) {
                            perror("Input file open failed");
                            exit(1);
                        }
                        dup2(fd_in, STDIN_FILENO);
                        close(fd_in);
                        // Remove '<' and filename from args
                        comm_args[in_redir_index] = NULL;
                    }

                    // Handle output redirection
                    if(out_redir_index != -1) {
                        char *outfile = comm_args[out_redir_index + 1];
                        int fd_out;

                        if(append_mode) {
                            // >> mode
                            fd_out = open(outfile, O_WRONLY | O_APPEND | O_CREAT, 0640);
                        } else {
                            // > mode
                            fd_out = creat(outfile, 0640);
                        }

                        if(fd_out < 0) {
                            perror("Output file creation failed");
                            exit(1);
                        }
                        dup2(fd_out, STDOUT_FILENO);
                        close(fd_out);

                        // Remove '>', '>>', and filename from arguments
                        comm_args[out_redir_index] = NULL;
                    }
                }

                // Execute the command (external program)
                execute_command(comm_args, arg_count);
                return 0; // Child ends after exec
            }
            else {
                // PARENT PROCESS
                if(k > 0) {
                    close(inp_init);
                }
                // If not the last command, prepare for next iteration
                if(k < piped_arg_count - 1) {
                    close(file_des[1]);
                    inp_init = file_des[0];
                }
                // If not a background process, wait for the child to complete
                if(!isAmps) {
                    int CStatus;
                    pid_t result = waitpid(comm_child_pid, &CStatus, 0);
                    if (result == -1) {
                        perror("waitpid() failed");
                    } else if (WIFEXITED(CStatus)) {
                        printf("\nChild exited with status: %d\n", WEXITSTATUS(CStatus));
                    }
                }
            }
        }
    }
    return 0;
}