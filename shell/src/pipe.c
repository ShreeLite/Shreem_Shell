#include "shell.h"
#include <sys/wait.h>
#include <fcntl.h>
#include <unistd.h>

/**
 * @brief Count the number of pipes in a command sequence
 * @param start_index Starting index in tokens array
 * @param end_index Ending index in tokens array  
 * @return Number of pipe operators found
 */
int count_pipes(int start_index, int end_index) {
    int pipe_count = 0;
    for (int i = start_index; i < end_index; i++) {
        if (tokens[i].type == TOKEN_PIPE) {
            pipe_count++;
        }
    }
    return pipe_count;
}

/**
 * @brief Find segments of commands separated by pipes
 * @param segments Pointer to array that will hold segment boundaries (will be allocated)
 * @param num_segments Pointer to store number of segments found
 * @return 0 on success, -1 on error
 * 
 * Each segment is represented by [start_index, end_index] pairs
 * Caller is responsible for freeing the segments array
 */
int find_pipe_segments(int **segments, int *num_segments) {
    int pipe_count = count_pipes(0, token_count);
    *num_segments = pipe_count + 1; // Number of commands = number of pipes + 1
    
    // Allocate memory for segment boundaries (2 integers per segment: start, end)
    *segments = malloc(*num_segments * 2 * sizeof(int));
    if (*segments == NULL) {
        perror("malloc segments");
        return -1;
    }
    
    int segment_index = 0;
    int current_start = 0;
    
    for (int i = 0; i <= token_count; i++) {
        // Found a pipe or reached end of tokens
        if (i == token_count || tokens[i].type == TOKEN_PIPE) {
            (*segments)[segment_index * 2] = current_start;     // start index
            (*segments)[segment_index * 2 + 1] = i;             // end index
            segment_index++;
            current_start = i + 1; // Next segment starts after the pipe
        }
    }
    
    return 0;
}

/**
 * @brief Execute a single command in a pipeline with appropriate pipe connections
 * @param cmd_start Start index of command tokens
 * @param cmd_end End index of command tokens
 * @param pipe_in File descriptor for input pipe (-1 if none)
 * @param pipe_out File descriptor for output pipe (-1 if none)
 * @param home_directory Shell home directory for built-in commands
 */
void execute_command_in_pipeline(int cmd_start, int cmd_end, int pipe_in, int pipe_out, const char* home_directory) {
    // Build argument array for execvp
    char *args[256];
    int arg_count = 0;
    
    // Extract command and arguments (skip redirection tokens)
    for (int i = cmd_start; i < cmd_end; i++) {
        if (tokens[i].type == TOKEN_NAME) {
            // Check if this is a redirection filename
            if (i > cmd_start && 
                (tokens[i-1].type == TOKEN_INPUT || 
                 tokens[i-1].type == TOKEN_OUTPUT || 
                 tokens[i-1].type == TOKEN_DOUBLE_OUTPUT)) {
                continue; // Skip redirection filenames
            }
            args[arg_count] = tokens[i].value;
            arg_count++;
        }
    }
    args[arg_count] = NULL;
    
    if (arg_count == 0) {
        printf("Error: No command found in pipeline segment\n");
        exit(1);
    }
    
    // Set up pipe connections first (before file redirections)
    if (pipe_in != -1) {
        if (dup2(pipe_in, STDIN_FILENO) == -1) {
            perror("dup2 pipe input");
            exit(1);
        }
        close(pipe_in);
    } else {
        // No pipe input - check if this command expects input and provide empty input
        // Commands like 'wc', 'grep', 'cat' without arguments expect stdin input
        if ((strcmp(args[0], "wc") == 0) || 
            (strcmp(args[0], "grep") == 0) ||
            (strcmp(args[0], "cat") == 0 && arg_count == 1)) {
            // Redirect stdin from /dev/null to provide empty input
            int devnull = open("/dev/null", O_RDONLY);
            if (devnull != -1) {
                dup2(devnull, STDIN_FILENO);
                close(devnull);
            }
        }
    }

    if (pipe_out != -1) {
        if (dup2(pipe_out, STDOUT_FILENO) == -1) {
            perror("dup2 pipe output");
            exit(1);
        }
        close(pipe_out);
    }

    // Set up file redirections (these can override pipe connections if specified)
    if (setup_input_redirection(cmd_start, cmd_end) == -1 ||
        setup_output_redirection(cmd_start, cmd_end) == -1) {
        exit(1);
    }

    // Handle built-in commands that can participate in pipes
    if (strcmp(args[0], "exit") == 0) {
        exit(0);
    } else if (strcmp(args[0], "pwd") == 0) {
        char cwd[MAX_PATH_LEN];
        if (getcwd(cwd, sizeof(cwd)) != NULL) {
            printf("%s\n", cwd);
        } else {
            perror("pwd");
        }
        exit(0);
    } else if (strcmp(args[0], "reveal") == 0) {
        // Create temporary tokens for reveal
        Token original_tokens[1024];
        int original_count = token_count;
        
        // Save original tokens
        memcpy(original_tokens, tokens, sizeof(Token) * token_count);
        
        // Copy segment tokens to the beginning of tokens array
        int segment_size = cmd_end - cmd_start;
        for (int i = 0; i < segment_size; i++) {
            tokens[i] = original_tokens[cmd_start + i];
        }
        tokens[segment_size].type = TOKEN_END;
        tokens[segment_size].value[0] = '\0';
        token_count = segment_size;
        
        // Execute reveal (home directory not available in pipeline, use NULL)
        execute_reveal(home_directory);
        
        // Restore original tokens
        memcpy(tokens, original_tokens, sizeof(Token) * original_count);
        token_count = original_count;
        
        exit(0);
    } else if (strcmp(args[0], "activities") == 0) {
        execute_activities();
        exit(0);
    }

    // Execute the external command
    execvp(args[0], args);
    printf("Command not found!\n"); // If execvp returns, there was an error
    exit(1);
}

//LLM GENERATED CODE STARTS HERE
/**
 * @brief Execute a complete pipeline of commands
 * @param home_directory Shell home directory for built-in commands
 * @return 0 on success, -1 on error
 * 
 * Requirements implemented:
 * - Creates pipes using pipe() system call for each | operator
 * - Forks child process for each command in pipeline
 * - Redirects stdout of command[i] to write end of pipe[i]
 * - Redirects stdin of command[i+1] to read end of pipe[i]
 * - Parent waits for all commands to complete
 * - File redirection works together with pipes
 * - Attempts to run remaining commands even if one fails
 */
int execute_pipeline(const char* home_directory) {
    if (token_count == 0) {
        return 0;
    }
    
    // Find pipe segments
    int *segments = NULL;
    int num_segments = 0;
    
    if (find_pipe_segments(&segments, &num_segments) == -1) {
        return -1;
    }
    
    // If only one segment and no pipes, use regular command execution
    if (num_segments == 1) {
        free(segments);
        return execute_command();
    }
    
    // Create pipes for pipeline
    int pipes[num_segments - 1][2]; // Need (num_segments - 1) pipes
    pid_t pids[num_segments];        // Store child PIDs
    
    // Create all pipes
    for (int i = 0; i < num_segments - 1; i++) {
        if (pipe(pipes[i]) == -1) {
            perror("pipe");
            free(segments);
            return -1;
        }
    }
    
    // Fork and execute each command in the pipeline
    for (int i = 0; i < num_segments; i++) {
        int cmd_start = segments[i * 2];
        int cmd_end = segments[i * 2 + 1];
        
        pids[i] = fork();
        
        if (pids[i] == 0) {
            // Child process
            
            // Close all pipe file descriptors that this process doesn't need
            for (int j = 0; j < num_segments - 1; j++) {
                if (j == i - 1) {
                    // This is the input pipe for current command
                    close(pipes[j][1]); // Close write end
                } else if (j == i) {
                    // This is the output pipe for current command
                    close(pipes[j][0]); // Close read end
                } else {
                    // Close both ends of other pipes
                    close(pipes[j][0]);
                    close(pipes[j][1]);
                }
            }
            
            // Determine pipe connections for this command
            int pipe_in = (i > 0) ? pipes[i - 1][0] : -1;           // Input from previous pipe
            int pipe_out = (i < num_segments - 1) ? pipes[i][1] : -1; // Output to next pipe
            
            // Execute the command with appropriate pipe connections
            execute_command_in_pipeline(cmd_start, cmd_end, pipe_in, pipe_out, home_directory);
            
            // Should not reach here if execvp succeeds
            exit(1);
            
        } else if (pids[i] == -1) {
            perror("fork");
            // Continue trying to fork other processes
        }
    }
    
    // Parent process: close all pipe file descriptors
    for (int i = 0; i < num_segments - 1; i++) {
        close(pipes[i][0]);
        close(pipes[i][1]);
    }
    
    // Wait for all child processes to complete
    int final_status = 0;
    for (int i = 0; i < num_segments; i++) {
        if (pids[i] > 0) { // Only wait for successfully forked processes
            int status;
            if (waitpid(pids[i], &status, 0) == -1) {
                perror("waitpid");
            } else {
                // Keep track of the last command's exit status
                if (i == num_segments - 1) {
                    final_status = WEXITSTATUS(status);
                }
            }
        }
    }
    
    free(segments);
    return final_status;
}

/**
 * @brief Check if the current command contains pipes
 * @return 1 if pipes are present, 0 otherwise
 */
int has_pipes(void) {
    return count_pipes(0, token_count) > 0;
}
//LLM GENERATED CODE ENDS HERE