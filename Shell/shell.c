#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define MAX_LINE_SIZE 255
#define WELCOME_MSG "Welcome to mini-shell.\n"
#define EXIT_MSG "Bye bye.\n"
#define PROMPT "shell $ "

char prev_cmd[MAX_LINE_SIZE] = {0};

void parse_and_execute_command(char *cmd) {
    char *args[MAX_LINE_SIZE / 2 + 1] = {0};
    char *p = cmd;
    int i = 0;

    // Parse the command string manually
    while (*p) {
        while (*p == ' ') p++; // Skip leading whitespace

        if (*p == '"') { // Handle double-quoted strings
            p++;
            args[i++] = p;
            while (*p && *p != '"') p++;
        } else {
            args[i++] = p;
            while (*p && *p != ' ' && *p != '"') p++;
        }

        if (*p) {
            *p = '\0';
            p++;
        }
    }

    if (args[0] != NULL) {
        // Create a child process to execute the command
        pid_t pid = fork();
        if (pid == -1) {
            perror("fork");
        } else if (pid == 0) {
            // Child process
            if (execvp(args[0], args) == -1) {
                fprintf(stderr, "%s: command not found\n", args[0]);
            }
            exit(EXIT_FAILURE); // Exit child process
        } else {
            // Parent process
            wait(NULL); // Wait for the child process to finish
        }
    }
}

void execute_command(char *cmd) {
    char *sequence = strtok(cmd, ";");
    while (sequence != NULL) {
        char cmd_copy[MAX_LINE_SIZE];
        strncpy(cmd_copy, sequence, MAX_LINE_SIZE - 1);
        cmd_copy[MAX_LINE_SIZE - 1] = '\0'; // Ensure null-termination
        parse_and_execute_command(cmd_copy); // Execute each command in the sequence
        sequence = strtok(NULL, ";");
    }
}

void execute_prev_command() {
    if (prev_cmd[0] != '\0') {
        printf("%s\n", prev_cmd);
        execute_command(prev_cmd);
    }
}

void test_execute_command() {
    // Test basic command execution
    char cmd1[] = "echo hello world";
    execute_command(cmd1);

    // Test input redirection
    char cmd2[] = "sort < words.txt";
    execute_command(cmd2);

    // Test output redirection
    char cmd3[] = "echo hello > greeting.txt";
    execute_command(cmd3);
    char cmd4[] = "cat greeting.txt";
    execute_command(cmd4);

    // Test piping
    char cmd5[] = "shuf -i 1-10 | sort -n";
    execute_command(cmd5);

    // Test changing directory
    char cmd6[] = "cd tmp";
    execute_command(cmd6);
    char cmd7[] = "pwd";
    execute_command(cmd7);

    // Test previous command
    char cmd8[] = "echo cs3650";
    execute_command(cmd8);
    char cmd9[] = "prev";
    execute_command(cmd9);
}

void test_execute_prev_command() {
    // Test executing previous command
    char cmd1[] = "echo hello world";
    execute_command(cmd1);
    char cmd2[] = "prev";
    execute_command(cmd2);
}

void test_exit_command() {
    // Test exit command
    char cmd1[] = "exit";
    execute_command(cmd1);
}

void test_invalid_command() {
    // Test invalid command
    char cmd1[] = "invalid_command";
    execute_command(cmd1);
}

int main() {
    char cmd[MAX_LINE_SIZE];
    printf(WELCOME_MSG);

    while (1) {
        printf(PROMPT);
        if (fgets(cmd, MAX_LINE_SIZE, stdin) == NULL) {
            printf(EXIT_MSG);
            break;
        }

        cmd[strcspn(cmd, "\n")] = 0; // Remove trailing newline

        if (strcmp(cmd, "exit") == 0) {
            printf(EXIT_MSG);
            break;
        } else if (strcmp(cmd, "prev") == 0) {
            execute_prev_command();
        } else {
            execute_command(cmd);
            strncpy(prev_cmd, cmd, MAX_LINE_SIZE - 1);
            prev_cmd[MAX_LINE_SIZE - 1] = '\0'; // Ensure null-termination
        }
    }

    return 0;
}
