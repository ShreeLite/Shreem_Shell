#include "shell.h"
#include <sys/wait.h>
#include <signal.h>
#include <fcntl.h>
//LLM GENERATED CODE BEGINS HERE
// Global background jobs array
BackgroundJob background_jobs[MAX_BACKGROUND_JOBS];
int next_job_number = 1;

/**
 * @brief Initialize the background jobs system
 */
void init_background_jobs(void) {
    for (int i = 0; i < MAX_BACKGROUND_JOBS; i++) {
        background_jobs[i].is_active = 0;
        background_jobs[i].pid = 0;
        background_jobs[i].job_number = 0;
        background_jobs[i].command[0] = '\0';
        background_jobs[i].command_name[0] = '\0';
        background_jobs[i].state = PROCESS_RUNNING;
    }
    next_job_number = 1;
}

/**
 * @brief Add a background job to the tracking system
 * @param pid Process ID of the background job
 * @param command Command string for display purposes
 * @return Job number assigned, or -1 if no slots available
 */
int add_background_job(pid_t pid, const char* command) {
    // Find an empty slot
    for (int i = 0; i < MAX_BACKGROUND_JOBS; i++) {
        if (!background_jobs[i].is_active) {
            background_jobs[i].pid = pid;
            background_jobs[i].job_number = next_job_number++;
            background_jobs[i].is_active = 1;
            background_jobs[i].state = PROCESS_RUNNING;
            strncpy(background_jobs[i].command, command, MAX_PATH_LEN - 1);
            background_jobs[i].command[MAX_PATH_LEN - 1] = '\0';
            
            // Extract command name (first word) for activities sorting
            strncpy(background_jobs[i].command_name, command, MAX_PATH_LEN - 1);
            background_jobs[i].command_name[MAX_PATH_LEN - 1] = '\0';
            char *space = strchr(background_jobs[i].command_name, ' ');
            if (space) {
                *space = '\0';
            }
            
            // Don't print job launch message to stdout to avoid interfering with command output
            
            return background_jobs[i].job_number;
        }
    }
    
    printf("Error: Maximum number of background jobs reached\n");
    return -1;
}

/**
 * @brief Add a stopped job to the tracking system (used by signal handlers)
 * @param pid Process ID of the stopped job
 * @param command Command string for display purposes
 * @return Job number assigned, or -1 if no slots available
 */
int add_stopped_job(pid_t pid, const char* command) {
    // Find an empty slot
    for (int i = 0; i < MAX_BACKGROUND_JOBS; i++) {
        if (!background_jobs[i].is_active) {
            background_jobs[i].pid = pid;
            background_jobs[i].job_number = next_job_number++;
            background_jobs[i].is_active = 1;
            background_jobs[i].state = PROCESS_STOPPED;
            strncpy(background_jobs[i].command, command, MAX_PATH_LEN - 1);
            background_jobs[i].command[MAX_PATH_LEN - 1] = '\0';
            
            // Extract command name (first word) for activities sorting
            strncpy(background_jobs[i].command_name, command, MAX_PATH_LEN - 1);
            background_jobs[i].command_name[MAX_PATH_LEN - 1] = '\0';
            char *space = strchr(background_jobs[i].command_name, ' ');
            if (space) {
                *space = '\0';
            }
            
            // Don't print launch message for stopped jobs
            
            return background_jobs[i].job_number;
        }
    }
    
    return -1; // No slots available
}

/**
 * @brief Check for completed background jobs and report their status
 */
void check_background_jobs(void) {
    int status;
    pid_t pid;
    
    // Check all active background jobs
    for (int i = 0; i < MAX_BACKGROUND_JOBS; i++) {
        if (background_jobs[i].is_active) {
            // Use WNOHANG to check without blocking, WUNTRACED to detect stopped processes
            pid = waitpid(background_jobs[i].pid, &status, WNOHANG | WUNTRACED);
            
            if (pid > 0) {
                char *command_name = background_jobs[i].command;
                
                if (WIFSTOPPED(status)) {
                    // Process was stopped (e.g., by SIGSTOP or Ctrl+Z)
                    // Note: No output required for stopped processes
                    background_jobs[i].state = PROCESS_STOPPED;
                } else if (WIFEXITED(status) || WIFSIGNALED(status)) {
                    // Process has terminated - print the full command with " &"
                    if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
                        printf("%s & with pid %d exited normally\n", command_name, background_jobs[i].pid);
                    } else {
                        printf("%s & with pid %d exited abnormally\n", command_name, background_jobs[i].pid);
                    }
                    
                    // Clean up this job slot for terminated processes
                    cleanup_background_job(i);
                }
                // If process continues running, leave it as is
            } else if (pid == -1) {
                // Error occurred - process might have already been reaped
                cleanup_background_job(i);
            }
            // If pid == 0, process is still running, continue
        }
    }
}

/**
 * @brief Clean up a background job slot
 * @param job_index Index in the background_jobs array
 */
void cleanup_background_job(int job_index) {
    if (job_index >= 0 && job_index < MAX_BACKGROUND_JOBS) {
        background_jobs[job_index].is_active = 0;
        background_jobs[job_index].pid = 0;
        background_jobs[job_index].job_number = 0;
        background_jobs[job_index].command[0] = '\0';
        background_jobs[job_index].command_name[0] = '\0';
        background_jobs[job_index].state = PROCESS_RUNNING;
    }
}

/**
 * @brief Execute a command in the background
 * @param start_index Starting token index for the command
 * @param end_index Ending token index for the command
 * @param command_str String representation of the command for tracking
 * @return 0 on success, -1 on error
 */
int execute_background_command(int start_index, int end_index, const char* command_str) {
    // Build argument array for the command
    char *args[256];
    int arg_count = 0;
    
    // Extract command and arguments (skip redirection tokens and filenames)
    for (int i = start_index; i < end_index; i++) {
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
                if (arg_count >= 255) break;
            }
        }
    }
    args[arg_count] = NULL;
    
    if (arg_count == 0) {
        printf("Error: No command found for background execution\n");
        return -1;
    }
    
    // Check if it's a built-in command (these can't run in background effectively)
    if (strcmp(args[0], "hop") == 0 || 
        strcmp(args[0], "reveal") == 0 || 
        strcmp(args[0], "log") == 0 ||
        strcmp(args[0], "activities") == 0 ||
        strcmp(args[0], "ping") == 0) {
        // Built-in commands cannot run in background - silently ignore
        return -1;
    }
    
    if (strcmp(args[0], "exit") == 0) {
        // exit command cannot run in background - silently ignore
        return -1;
    }
    
    // Fork a child process for background execution
    pid_t pid = fork();
    
    if (pid == 0) {
        // Child process
        
        // Redirect stdin to /dev/null for background processes
        int null_fd = open("/dev/null", O_RDONLY);
        if (null_fd != -1) {
            dup2(null_fd, STDIN_FILENO);
            close(null_fd);
        }
        
        // Set up output redirection if specified
        if (setup_output_redirection(start_index, end_index) == -1) {
            exit(1);
        }
        
        // Set up input redirection if specified (though uncommon for background)
        if (setup_input_redirection(start_index, end_index) == -1) {
            exit(1);
        }
        
        // Execute the command
        execvp(args[0], args);
        
        // If execvp returns, there was an error
        printf("Command not found!\n");
        exit(127);
    } else if (pid > 0) {
        // Parent process - don't wait, just add to background jobs
        add_background_job(pid, command_str);
        return 0;
    } else {
        perror("fork");
        return -1;
    }
}

//LLM GENERATED CODE ENDS HERE
