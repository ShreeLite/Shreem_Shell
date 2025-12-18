#include "shell.h"

// This variable is defined here and declared in shell.h
char previous_cwd[MAX_PATH_LEN] = ""; // Initialize to empty

/**
 * @brief Executes the built-in 'hop' command.
 * @param home_directory The shell's starting home directory.
 */
void execute_hop(const char* home_directory) {
    // If no arguments are given (token_count is 1: just "hop"), behave like "hop ~"
    if (token_count <= 1) {
        // Before changing, store the current directory
        if (getcwd(previous_cwd, sizeof(previous_cwd)) == NULL) {
            perror("getcwd error");
        }
        if (chdir(home_directory) != 0) {
            perror("hop");
        }
        return;
    }

    // Loop through all arguments starting from the second token
    for (int i = 1; i < token_count; i++) {
        char* arg = tokens[i].value;
        char current_cwd_buffer[MAX_PATH_LEN];

        // Get the current CWD before any potential change
        if (getcwd(current_cwd_buffer, sizeof(current_cwd_buffer)) == NULL) {
            perror("getcwd error");
            continue; // Skip to next argument on error
        }

        if (strcmp(arg, "~") == 0) {
            strcpy(previous_cwd, current_cwd_buffer); // Save current before hopping
            if (chdir(home_directory) != 0) {
                printf("No such directory!\n");
            }
        } else if (strcmp(arg, ".") == 0) {
            // Do nothing
            continue;
        } else if (strcmp(arg, "..") == 0) {
            strcpy(previous_cwd, current_cwd_buffer); // Save current before hopping
            if (chdir("..") != 0) {
                printf("No such directory!\n");
            }
        } else if (strcmp(arg, "-") == 0) {
            if (strlen(previous_cwd) == 0) {
                // Do nothing if there is no previous directory
                continue;
            }
            // Swap current and previous directories
            char temp_cwd[MAX_PATH_LEN];
            strcpy(temp_cwd, current_cwd_buffer);
            if (chdir(previous_cwd) != 0) {
                printf("No such directory!\n");
            } else {
                strcpy(previous_cwd, temp_cwd);
            }

        } else { // It's a path name
            if (chdir(arg) != 0) {
                printf("No such directory!\n");
            } else {
                // Only update previous_cwd on a successful hop to a named dir
                strcpy(previous_cwd, current_cwd_buffer);
            }
        }
    }
}
