#include "shell.h"
#include <sys/wait.h>
#include <fcntl.h>

/**
 * @brief Execute an external command with given arguments
 * @param args Array of arguments (command and its parameters), NULL-terminated
 * @return Exit status of the command, -1 on error
 */
int execute_external_command(char **args) {
    if (args == NULL || args[0] == NULL) {
        printf("Error: No command specified\n");
        return -1;
    }
    
    // Fork a child process
    pid_t pid = fork();
    
    if (pid == 0) {
        // Child process: execute the command
        execvp(args[0], args);
        
        // If execvp returns, there was an error
        perror(args[0]);
        exit(127); // Use exit code 127 for "command not found"
    } else if (pid > 0) {
        // Parent process: wait for child to complete
        int status;
        if (waitpid(pid, &status, 0) == -1) {
            perror("waitpid");
            return -1;
        }
        
        // Return the exit status of the child process
        if (WIFEXITED(status)) {
            return WEXITSTATUS(status);
        } else if (WIFSIGNALED(status)) {
            // Child was terminated by a signal
            return 128 + WTERMSIG(status);
        } else {
            return -1;
        }
    } else {
        // Fork failed
        perror("fork");
        return -1;
    }
}

/**
 * @brief Parse tokens and execute the command with proper argument handling
 * @return Exit status of the executed command
 */
int execute_command(void) {
    if (token_count == 0) {
        return 0;
    }
    
    // Build argument array for the command, excluding redirection parts
    char *args[256]; // Maximum 256 arguments
    int arg_count = 0;
    
    // Find the boundaries of the command (stop at pipes, semicolons, ampersands)
    int cmd_end = token_count;
    for (int i = 0; i < token_count; i++) {
        if (tokens[i].type == TOKEN_PIPE || 
            tokens[i].type == TOKEN_SEMICOLON || 
            tokens[i].type == TOKEN_AMPERSAND) {
            cmd_end = i;
            break;
        }
    }
    
    // Extract command and arguments (skip redirection tokens and filenames)
    for (int i = 0; i < cmd_end; i++) {
        if (tokens[i].type == TOKEN_NAME) {
            // Check if this token is a redirection filename
            bool is_redirect_file = false;
            if (i > 0) {
                TokenType prev_type = tokens[i-1].type;
                if (prev_type == TOKEN_INPUT || 
                    prev_type == TOKEN_OUTPUT || 
                    prev_type == TOKEN_DOUBLE_OUTPUT) {
                    is_redirect_file = true;
                }
            }
            
            // If it's not a redirection filename, add it as a command argument
            if (!is_redirect_file) {
                args[arg_count] = tokens[i].value;
                arg_count++;
                if (arg_count >= 255) break; // Prevent overflow
            }
        }
    }
    args[arg_count] = NULL; // NULL-terminate the argument array
    
    if (arg_count == 0) {
        printf("Error: No command found\n");
        return -1;
    }
    
    // Check if it's a built-in command
    if (strcmp(args[0], "hop") == 0 || 
        strcmp(args[0], "reveal") == 0 || 
        strcmp(args[0], "exit") == 0) {
        printf("Built-in commands should be handled separately\n");
        return 0;
    }
    
    // Fork a child process for external command execution
    pid_t pid = fork();
    
    if (pid == 0) {
        // Child process: set up process group and redirections
        
        // Create new process group for job control
        setpgid(0, 0);
        
        // Set up input redirection
        if (setup_input_redirection(0, cmd_end) == -1) {
            exit(1);
        }
        
        // Set up output redirection
        if (setup_output_redirection(0, cmd_end) == -1) {
            exit(1);
        }
        
        // Execute the command
        execvp(args[0], args);
        
        // If execvp returns, there was an error
        printf("Command not found!\n");
        exit(127);
    } else if (pid > 0) {
        // Parent process: set up foreground tracking and wait for child
        
        // Set child as its own process group leader
        setpgid(pid, pid);
        
        // Track this as the current foreground process
        set_foreground_process(pid, args[0]);
        
        int status;
        if (waitpid(pid, &status, WUNTRACED) == -1) {
            perror("waitpid");
            clear_foreground_process();
            return -1;
        }
        
        // Clear foreground tracking
        clear_foreground_process();
        
        if (WIFEXITED(status)) {
            return WEXITSTATUS(status);
        } else if (WIFSIGNALED(status)) {
            return 128 + WTERMSIG(status);
        } else if (WIFSTOPPED(status)) {
            // Process was stopped - it should be handled by signal handler
            return 0;
        } else {
            return -1;
        }
    } else {
        perror("fork");
        return -1;
    }
}