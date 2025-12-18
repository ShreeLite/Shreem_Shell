#include "shell.h" 
#include <sys/wait.h>
#include <fcntl.h>

// Function to execute a single atomic command (with redirections)
int execute_atomic_command(int start_index, int end_index) {
    // Build argument array for execvp
    char *args[256]; // Max 256 arguments
    int arg_count = 0;
    
    // Extract command and arguments (skip redirection tokens)
    for (int i = start_index; i < end_index; i++) {
        if (tokens[i].type == TOKEN_NAME) {
            // Check if this is a redirection filename (preceded by redirection token)
            if (i > start_index && 
                (tokens[i-1].type == TOKEN_INPUT || 
                 tokens[i-1].type == TOKEN_OUTPUT || 
                 tokens[i-1].type == TOKEN_DOUBLE_OUTPUT)) {
                continue; // Skip redirection filenames
            }
            args[arg_count] = tokens[i].value;
            arg_count++;
        }
    }
    args[arg_count] = NULL; // Null-terminate for execvp
    
    if (arg_count == 0) {
        printf("Error: No command found\n");
        return -1;
    }
    
    // Check if it's a built-in command
    if (strcmp(args[0], "hop") == 0) {
        // For built-ins, we don't fork - just execute directly
        // But we need to handle the fact that we're already parsing differently
        printf("Built-in commands should be handled in main execution flow\n");
        return 0;
    } else if (strcmp(args[0], "reveal") == 0) {
        printf("Built-in commands should be handled in main execution flow\n");
        return 0;
    } else if (strcmp(args[0], "exit") == 0) {
        exit(0);
    } else if (strcmp(args[0], "pwd") == 0) {
        char cwd[MAX_PATH_LEN];
        if (getcwd(cwd, sizeof(cwd)) != NULL) {
            printf("%s\n", cwd);
        } else {
            perror("pwd");
        }
        return 0;
    }
    
    // Fork and exec for external commands
    pid_t pid = fork();
    if (pid == 0) {
        // Child process
        
        // Set up redirections
        if (setup_input_redirection(start_index, end_index) == -1 ||
            setup_output_redirection(start_index, end_index) == -1) {
            exit(1);
        }
        
        // Execute the command
        execvp(args[0], args);
        printf("Command not found!\n"); // If execvp returns, there was an error
        exit(1);
    } else if (pid > 0) {
        // Parent process - wait for child
        int status;
        waitpid(pid, &status, 0);
        return WEXITSTATUS(status);
    } else {
        perror("fork");
        return -1;
    }
}

//  Main Shell Loop 
int main() {
    char home_directory[MAX_PATH_LEN];
    
    // The directory in which the shell is started becomes the shell's home directory
    if (getcwd(home_directory, sizeof(home_directory)) == NULL) {
        perror("Failed to get shell home directory");
        return 1;
    }

    // Initialize the log system and background jobs
    init_log(home_directory);
    init_background_jobs();
    
    // Setup signal handlers for job control (Ctrl-C, Ctrl-Z)
    setup_signal_handlers();

    char input_buffer[MAX_PATH_LEN];

    // The main Read-Eval-Print-Loop (REPL)
    while (1) {
        // Check for completed background jobs before displaying prompt
        check_background_jobs();
        
        prompt(home_directory);

        if (fgets(input_buffer, sizeof(input_buffer), stdin) == NULL) {
            // Handle Ctrl-D (EOF)
            handle_eof();
        }

        if (input_buffer[0] == '\n') {
            continue;
        }

        // Check for completed background jobs before processing new command
        check_background_jobs();

        current_input = input_buffer;
        tokenise();
        
        // Parse the tokens
        int parse_result = parse();
        if (parse_result) {
            // Execute sequential/background commands
            execute_sequential_commands(home_directory);
        } else {
            printf("Invalid Syntax!\n");
        }
    }

    printf("Shell terminated.\n");
    return 0;
}
