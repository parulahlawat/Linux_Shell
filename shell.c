/*
 * Linux Shell in C
 * Features: piping, I/O redirection, background jobs, alias, history,
 *           signal handling, color prompt, echo with $VAR expansion
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <signal.h>

/* ─── ANSI color codes for the prompt ─── 
   \001 and \002 tell readline to ignore these bytes when calculating
   cursor position — required on macOS to prevent garbled prompts */
#define COLOR_GREEN   "\001\033[1;32m\002"
#define COLOR_BLUE    "\001\033[1;34m\002"
#define COLOR_RESET   "\001\033[0m\002"
#define COLOR_CYAN    "\001\033[1;36m\002"

/* ─── Restricted words for output redirection ─── */
const char* restricted_words[] = {"kill", "death"};
const int num_restricted = sizeof(restricted_words) / sizeof(restricted_words[0]);

/* ─── Alias support (Commit 2) ─── */
#define MAX_ALIASES 64
typedef struct {
    char name[64];
    char value[256];
} Alias;
Alias alias_list[MAX_ALIASES];
int alias_count = 0;

void add_alias(const char *name, const char *value) {
    /* Update existing alias if name matches */
    for (int i = 0; i < alias_count; i++) {
        if (strcmp(alias_list[i].name, name) == 0) {
            strncpy(alias_list[i].value, value, 255);
            return;
        }
    }
    if (alias_count < MAX_ALIASES) {
        strncpy(alias_list[alias_count].name, name, 63);
        strncpy(alias_list[alias_count].value, value, 255);
        alias_count++;
    }
}

void remove_alias(const char *name) {
    for (int i = 0; i < alias_count; i++) {
        if (strcmp(alias_list[i].name, name) == 0) {
            alias_list[i] = alias_list[alias_count - 1];
            alias_count--;
            return;
        }
    }
    fprintf(stderr, "unalias: %s: not found\n", name);
}

/* Expand alias: if first token matches an alias, replace it */
void expand_alias(char *command, size_t size) {
    char temp[1024];
    strncpy(temp, command, sizeof(temp));
    char *first = strtok(temp, " \t");
    if (!first) return;
    for (int i = 0; i < alias_count; i++) {
        if (strcmp(alias_list[i].name, first) == 0) {
            char expanded[1024];
            const char *rest = command + strlen(first);
            snprintf(expanded, sizeof(expanded), "%s%s", alias_list[i].value, rest);
            strncpy(command, expanded, size - 1);
            command[size - 1] = '\0';
            return;
        }
    }
}

/* ─── Background job tracking (Commit 5) ─── */
#define MAX_JOBS 64
typedef struct {
    pid_t pid;
    char  cmd[256];
    int   active;
} Job;
Job job_list[MAX_JOBS];
int job_count = 0;

void add_job(pid_t pid, const char *cmd) {
    for (int i = 0; i < MAX_JOBS; i++) {
        if (!job_list[i].active) {
            job_list[i].pid    = pid;
            job_list[i].active = 1;
            strncpy(job_list[i].cmd, cmd, 255);
            job_count++;
            printf("[%d] %d\n", i + 1, pid);
            return;
        }
    }
}

void reap_background_jobs(void) {
    int status;
    for (int i = 0; i < MAX_JOBS; i++) {
        if (!job_list[i].active) continue;
        pid_t result = waitpid(job_list[i].pid, &status, WNOHANG);
        if (result > 0) {
            printf("\n[Done] PID %d: %s\n", job_list[i].pid, job_list[i].cmd);
            job_list[i].active = 0;
            job_count--;
        }
    }
}

void print_jobs(void) {
    int found = 0;
    for (int i = 0; i < MAX_JOBS; i++) {
        if (job_list[i].active) {
            printf("[%d] Running   PID %d   %s\n", i + 1, job_list[i].pid, job_list[i].cmd);
            found = 1;
        }
    }
    if (!found) printf("No background jobs.\n");
}

/* ─── Signal handling: Ctrl+C does NOT kill the shell (Commit 4) ─── */
void sigint_handler(int sig) {
    (void)sig;
    printf("\n");
    rl_on_new_line();
    rl_redisplay();
}

/* ─── $VAR expansion for echo (Commit 3) ─── */
void expand_env_vars(char *arg, size_t size) {
    if (arg[0] != '$') return;
    const char *val = getenv(arg + 1);
    if (val) {
        strncpy(arg, val, size - 1);
        arg[size - 1] = '\0';
    }
}

/* ─── Color prompt builder (Commit 6) ─── */
char *build_prompt(void) {
    static char prompt[512];
    char cwd[256];
    char hostname[128];
    char *user = getenv("USER");
    if (!user) user = "user";
    gethostname(hostname, sizeof(hostname));
    getcwd(cwd, sizeof(cwd));

    /* Replace HOME path with ~ */
    char *home = getenv("HOME");
    char display_cwd[256];
    if (home && strncmp(cwd, home, strlen(home)) == 0) {
        snprintf(display_cwd, sizeof(display_cwd), "~%s", cwd + strlen(home));
    } else {
        strncpy(display_cwd, cwd, sizeof(display_cwd));
    }

    snprintf(prompt, sizeof(prompt),
        "\n" COLOR_GREEN "%s@%s" COLOR_RESET ":" COLOR_BLUE "%s" COLOR_RESET COLOR_CYAN "$ " COLOR_RESET,
        user, hostname, display_cwd);
    return prompt;
}

/* ─── Execute command ─── */
void execute_command(char *args[], int n) {
    (void)n;
    execvp(args[0], args);
    perror("Execvp() failed");
    exit(EXIT_FAILURE);
}

/* ─── Main ─── */
int main(void) {
    using_history();

    /* Commit 4: register SIGINT handler */
    signal(SIGINT, sigint_handler);

    /* Initialize job list */
    memset(job_list, 0, sizeof(job_list));

    while (1) {
        /* Commit 5: reap finished background jobs at each prompt */
        reap_background_jobs();

        char piped_command[1024] = "";
        short isAmps = 0;

        /* Commit 6: color prompt */
        char *line = readline(build_prompt());
        if (!line) {
            printf("\n");
            break; /* EOF (Ctrl+D) */
        }
        strcat(piped_command, line);
        free(line);

        /* Multi-line commands with backslash */
        while (strlen(piped_command) > 0 &&
               piped_command[strlen(piped_command) - 1] == '\\') {
            piped_command[strlen(piped_command) - 1] = '\0';
            char *cont = readline("> ");
            if (!cont) break;
            strcat(piped_command, cont);
            free(cont);
        }

        if (!strlen(piped_command)) continue;
        add_history(piped_command);

        /* Commit 2: expand alias before anything else */
        expand_alias(piped_command, sizeof(piped_command));

        /* ── Built-ins that need the full raw command ── */

        /* Commit 7: history built-in — uses history_get() (works on both
           Apple readline and GNU readline) */
        if (strcmp(piped_command, "history") == 0) {
            for (int i = 0; i < history_length; i++) {
                HIST_ENTRY *entry = history_get(history_base + i);
                if (entry && entry->line)
                    printf("  %3d  %s\n", i + 1, entry->line);
            }
            continue;
        }

        /* Commit 5: jobs built-in */
        if (strcmp(piped_command, "jobs") == 0) {
            print_jobs();
            continue;
        }

        /* Commit 2: alias / unalias built-ins */
        if (strncmp(piped_command, "alias", 5) == 0 &&
            (piped_command[5] == ' ' || piped_command[5] == '\0')) {
            char *rest = piped_command + 5;
            while (*rest == ' ') rest++;
            if (*rest == '\0') {
                for (int i = 0; i < alias_count; i++)
                    printf("alias %s='%s'\n", alias_list[i].name, alias_list[i].value);
            } else {
                char *eq = strchr(rest, '=');
                if (eq) {
                    char aname[64] = {0};
                    char avalue[256] = {0};
                    int nlen = eq - rest;
                    if (nlen > 63) nlen = 63;
                    strncpy(aname, rest, nlen);
                    strncpy(avalue, eq + 1, 255);
                    /* Strip one layer of surrounding quotes */
                    int vlen = strlen(avalue);
                    if (vlen >= 2 &&
                        ((avalue[0] == '\'' && avalue[vlen-1] == '\'') ||
                         (avalue[0] == '"'  && avalue[vlen-1] == '"'))) {
                        avalue[vlen-1] = '\0';
                        memmove(avalue, avalue+1, vlen);
                    }
                    add_alias(aname, avalue);
                    printf("Alias added: %s -> %s\n", aname, avalue);
                } else {
                    int found = 0;
                    for (int i = 0; i < alias_count; i++) {
                        if (strcmp(alias_list[i].name, rest) == 0) {
                            printf("alias %s='%s'\n", alias_list[i].name, alias_list[i].value);
                            found = 1;
                        }
                    }
                    if (!found) fprintf(stderr, "alias: %s: not found\n", rest);
                }
            }
            continue;
        }

        if (strncmp(piped_command, "unalias", 7) == 0) {
            char *rest = piped_command + 7;
            while (*rest == ' ') rest++;
            if (*rest) remove_alias(rest);
            else fprintf(stderr, "Usage: unalias name\n");
            continue;
        }

        /* ── Pipe splitting ── */
        char* piped_comm_args[100];
        int piped_arg_count = 0;
        char *Ptoken = strtok(piped_command, "|");
        while (Ptoken != NULL) {
            piped_comm_args[piped_arg_count++] = Ptoken;
            Ptoken = strtok(NULL, "|");
        }

        int file_des[2];
        int inp_init = 0;

        for (int k = 0; k < piped_arg_count; k++) {
            char command[1024] = "";
            strcat(command, piped_comm_args[k]);

            /* Trim leading spaces */
            char *cmd_ptr = command;
            while (*cmd_ptr == ' ') cmd_ptr++;
            if (!strlen(cmd_ptr)) continue;
            memmove(command, cmd_ptr, strlen(cmd_ptr) + 1);

            char *comm_args[100];
            int arg_count = 0;
            char *token = strtok(command, " \t");
            while (token != NULL) {
                comm_args[arg_count++] = token;
                token = strtok(NULL, " \t");
            }

            if (arg_count == 0) continue;

            /* Background & detection */
            if (comm_args[arg_count-1] &&
                comm_args[arg_count-1][strlen(comm_args[arg_count-1])-1] == '&') {
                isAmps = 1;
                if (strlen(comm_args[arg_count-1]) == 1)
                    arg_count--;
                else
                    comm_args[arg_count-1][strlen(comm_args[arg_count-1])-1] = '\0';
            }

            comm_args[arg_count] = NULL;

            if (k < piped_arg_count - 1) {
                if (pipe(file_des) == -1) {
                    perror("Pipe Error");
                    exit(EXIT_FAILURE);
                }
            }

            /* ── Built-ins ── */
            if (strcmp(comm_args[0], "exit") == 0) exit(0);

            if (strcmp(comm_args[0], "help") == 0) {
                printf("\n" COLOR_CYAN "Commands Available:" COLOR_RESET "\n");
                printf("  Built-in : pwd, cd <dir>, mkdir <dir>, ls, exit, help\n");
                printf("  New      : alias, unalias, jobs, history\n");
                printf("  Features : piping (|), redirection (<, >, >>), background (&)\n");
                printf("  Tips     : Ctrl+C cancels current command, up-arrow for history\n");
                continue;
            }

            if (strcmp(comm_args[0], "cd") == 0) {
                char s1[256], s2[256], temp[1024] = "";
                getcwd(s1, sizeof(s1));
                if (comm_args[1]) {
                    strcat(temp, comm_args[1]);
                    for (int i = 2; comm_args[i] != NULL; i++) {
                        strcat(temp, " ");
                        strcat(temp, comm_args[i]);
                    }
                } else {
                    /* cd with no args goes home */
                    char *home = getenv("HOME");
                    if (home) strcat(temp, home);
                }
                if (chdir(temp) == 0) {
                    printf("\nPWD: %s  →  %s\n", s1, getcwd(s2, sizeof(s2)));
                } else {
                    perror("cd failed");
                }
                continue;
            }

            /* Commit 3: echo with $VAR expansion */
            if (strcmp(comm_args[0], "echo") == 0) {
                for (int i = 1; comm_args[i] != NULL; i++) {
                    char arg_copy[256];
                    strncpy(arg_copy, comm_args[i], 255);
                    expand_env_vars(arg_copy, sizeof(arg_copy));
                    printf("%s ", arg_copy);
                }
                printf("\n");
                continue;
            }

            /* ── Fork & exec ── */
            pid_t comm_child_pid = fork();
            if (comm_child_pid == -1) {
                perror("Fork() failed");
                exit(1);
            } else if (comm_child_pid == 0) {
                /* Child: restore default SIGINT so child can be interrupted */
                signal(SIGINT, SIG_DFL);

                if (k > 0) {
                    dup2(inp_init, STDIN_FILENO);
                    close(inp_init);
                }
                if (k < piped_arg_count - 1) {
                    dup2(file_des[1], STDOUT_FILENO);
                    close(file_des[0]);
                    close(file_des[1]);
                }

                int in_redir_index = -1, out_redir_index = -1, append_mode = 0;
                for (int j = 0; j < arg_count; j++) {
                    if (!comm_args[j]) continue;
                    if (strcmp(comm_args[j], "<") == 0)  { in_redir_index  = j; }
                    else if (strcmp(comm_args[j], ">>") == 0) { out_redir_index = j; append_mode = 1; }
                    else if (strcmp(comm_args[j], ">") == 0)  { out_redir_index = j; append_mode = 0; }
                }

                if (in_redir_index != -1 && comm_args[in_redir_index + 1]) {
                    int fd_in = open(comm_args[in_redir_index + 1], O_RDONLY);
                    if (fd_in < 0) { perror("Input file open failed"); exit(1); }
                    dup2(fd_in, STDIN_FILENO);
                    close(fd_in);
                    comm_args[in_redir_index] = NULL;
                }

                if (out_redir_index != -1 && comm_args[out_redir_index + 1]) {
                    /* Check restricted words */
                    int hasRestricted = 0;
                    for (int i = 1; comm_args[i] != NULL && i < out_redir_index; i++) {
                        for (int j = 0; j < num_restricted; j++) {
                            if (strstr(comm_args[i], restricted_words[j])) {
                                fprintf(stderr, "❌ Restricted word detected: \"%s\" — blocked.\n", restricted_words[j]);
                                hasRestricted = 1;
                                break;
                            }
                        }
                        if (hasRestricted) break;
                    }
                    if (hasRestricted) exit(1);

                    int fd_out = append_mode
                        ? open(comm_args[out_redir_index+1], O_WRONLY|O_APPEND|O_CREAT, 0640)
                        : creat(comm_args[out_redir_index+1], 0640);
                    if (fd_out < 0) { perror("Output file open failed"); exit(1); }
                    dup2(fd_out, STDOUT_FILENO);
                    close(fd_out);
                    comm_args[out_redir_index] = NULL;
                }

                execute_command(comm_args, arg_count);
                return 0;
            } else {
                /* Parent */
                if (k > 0) close(inp_init);
                if (k < piped_arg_count - 1) {
                    close(file_des[1]);
                    inp_init = file_des[0];
                }
                if (isAmps) {
                    /* Commit 5: track background job */
                    add_job(comm_child_pid, comm_args[0]);
                } else {
                    int CStatus;
                    pid_t result = waitpid(comm_child_pid, &CStatus, 0);
                    if (result == -1) perror("waitpid() failed");
                    else if (WIFEXITED(CStatus))
                        printf("\nChild exited with status: %d\n", WEXITSTATUS(CStatus));
                }
            }
        }
    }
    return 0;
}
