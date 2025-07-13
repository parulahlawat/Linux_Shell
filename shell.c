#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <sys/wait.h>
#include <fcntl.h>

const char* restricted_words[] = {"kill", "death"};
const int num_restricted = sizeof(restricted_words) / sizeof(restricted_words[0]);

int max(int a, int b) {
    return (a > b) ? a : b;
}

void execute_command(char* args[], int n) {
    int execvp_code = execvp(args[0], args);
    if(execvp_code == -1) {
        perror("Execvp() failed");
        exit(EXIT_FAILURE);
    }
}

int count_words(char* line) {
    int word_count = 0;
    char *token = strtok(line, " ");
    while(token != NULL) {
        token = strtok(NULL, " ");
        word_count++;
    }
    return word_count;
}

int main() {
    using_history();

    while (1) {
        char piped_command[1024] = "";
        short isAmps = 0;

        strcat(piped_command, readline("\nShell>"));
        while (piped_command[strlen(piped_command)-1] == '\\') {
            piped_command[strlen(piped_command)-1] = '\0';
            strcat(piped_command, readline("\n>"));
        }

        if(!strlen(piped_command)) continue;
        add_history(piped_command);

        char* piped_comm_args[100];
        int piped_arg_count = 0;
        char *Ptoken = strtok(piped_command, "|");
        while(Ptoken != NULL) {
            piped_comm_args[piped_arg_count] = Ptoken;
            Ptoken = strtok(NULL, "|");
            piped_arg_count++;
        }

        int file_des[2];
        int inp_init = 0;

        for(int k = 0; k < piped_arg_count; k++) {
            char command[1024] = "";
            strcat(command, piped_comm_args[k]);
            if(!strlen(command)) continue;

            char* comm_args[100];
            int arg_count = 0;
            char *token = strtok(command, " ");
            while(token != NULL) {
                comm_args[arg_count] = token;
                token = strtok(NULL, " ");
                arg_count++;
            }

            if(comm_args[arg_count-1] &&
               comm_args[arg_count-1][strlen(comm_args[arg_count-1]) - 1] == '&') {
                isAmps = 1;
                if(strlen(comm_args[arg_count-1]) == 1)
                    arg_count--;
                else
                    comm_args[arg_count-1][strlen(comm_args[arg_count-1]) - 1] = '\0';
            }

            comm_args[arg_count] = NULL;
            arg_count++;

            if(k < piped_arg_count - 1) {
                if(pipe(file_des) == -1) {
                    perror("Pipe Error");
                    exit(EXIT_FAILURE);
                }
            }

            if(strcmp(comm_args[0], "exit") == 0) exit(0);
            if(strcmp(comm_args[0], "help") == 0) {
                printf("\nCommands Available:\n");
                printf("\tpwd, cd <dir>, mkdir <dir>, ls, exit, help, external commands\n");
                continue;
            }
            if(strcmp(comm_args[0], "cd") == 0) {
                char s1[100], s2[100], temp[1024] = "";
                getcwd(s1, 100);
                strcat(temp, comm_args[1]);
                for(int i = 2; i < arg_count - 1; i++) {
                    strcat(temp, " ");
                    strcat(temp, comm_args[i]);
                }
                if(chdir(temp) == 0) {
                    printf("\nPWD changed from: %s\n", s1);
                    printf("To: %s\n", getcwd(s2, 100));
                } else {
                    perror("cd command failed");
                }
                continue;
            }

            pid_t comm_child_pid = fork();
            if(comm_child_pid == -1) {
                perror("Fork() failed");
                exit(1);
            }
            else if(comm_child_pid == 0) {
                printf("\nInside Child Process with PID: %d\n", getpid());
                printf("Executing Command: %s\n\n", comm_args[0]);

                if(k > 0) {
                    dup2(inp_init, STDIN_FILENO);
                    close(inp_init);
                }
                if(k < piped_arg_count - 1) {
                    dup2(file_des[1], STDOUT_FILENO);
                    close(file_des[0]);
                    close(file_des[1]);
                }

                int in_redir_index = -1, out_redir_index = -1, append_mode = 0;
                for(int j = 0; j < arg_count - 1; j++) {
                    if(comm_args[j] && strcmp(comm_args[j], "<") == 0) {
                        in_redir_index = j;
                    } else if(comm_args[j] && strcmp(comm_args[j], ">>") == 0) {
                        out_redir_index = j;
                        append_mode = 1;
                    } else if(comm_args[j] && strcmp(comm_args[j], ">") == 0) {
                        out_redir_index = j;
                        append_mode = 0;
                    }
                }

                if(in_redir_index != -1) {
                    char *infile = comm_args[in_redir_index + 1];
                    int fd_in = open(infile, O_RDONLY);
                    if(fd_in < 0) {
                        perror("Input file open failed");
                        exit(1);
                    }
                    dup2(fd_in, STDIN_FILENO);
                    close(fd_in);
                    comm_args[in_redir_index] = NULL;
                }

                if(out_redir_index != -1) {
                    // Check for restricted words before writing
                    int hasRestricted = 0;
                    for(int i = 1; comm_args[i] != NULL; i++) {
                        for(int j = 0; j < num_restricted; j++) {
                            if (strstr(comm_args[i], restricted_words[j]) != NULL) {
                                fprintf(stderr, "❌ Restricted word detected: \"%s\" — command blocked.\n", restricted_words[j]);
                                hasRestricted = 1;
                                break;
                            }
                        }
                        if (hasRestricted) break;
                    }
                    if (hasRestricted) exit(1);

                    char *outfile = comm_args[out_redir_index + 1];
                    int fd_out = append_mode ? open(outfile, O_WRONLY | O_APPEND | O_CREAT, 0640) : creat(outfile, 0640);
                    if(fd_out < 0) {
                        perror("Output file creation failed");
                        exit(1);
                    }
                    dup2(fd_out, STDOUT_FILENO);
                    close(fd_out);
                    comm_args[out_redir_index] = NULL;
                }

                execute_command(comm_args, arg_count);
                return 0;
            }
            else {
                if(k > 0) close(inp_init);
                if(k < piped_arg_count - 1) {
                    close(file_des[1]);
                    inp_init = file_des[0];
                }
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