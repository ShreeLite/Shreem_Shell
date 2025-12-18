#include "shell.h"
#include <fcntl.h>
#include <unistd.h>

/**
 * @brief Set up input redirection for a command
 * @param start_index Starting index in the tokens array
 * @param end_index Ending index in the tokens array
 * @return 0 on success, -1 on error
 * 
 * Requirements implemented:
 * - Uses open() with O_RDONLY flag
 * - Prints "No such file or directory" if file doesn't exist
 * - Uses dup2() to redirect STDIN_FILENO
 * - Closes original file descriptor after duplication
 * - Handles multiple input redirections (last one takes effect)
 */
int setup_input_redirection(int start_index, int end_index) {
    char *input_filename = NULL;
    int input_fd = -1;
    
    // Scan through tokens to find input redirection
    // If multiple input redirections are present, only the last one takes effect
    for (int i = start_index; i < end_index; i++) {
        if (tokens[i].type == TOKEN_INPUT) {
            // Check if there's a filename after the < token
            if (i + 1 < end_index && tokens[i + 1].type == TOKEN_NAME) {
                // Close previous input file descriptor if it exists
                if (input_fd != -1) {
                    close(input_fd);
                }
                
                input_filename = tokens[i + 1].value;
                
                // Open the input file for reading
                input_fd = open(input_filename, O_RDONLY);
                
                if (input_fd == -1) {
                    // File doesn't exist or cannot be opened
                    printf("No such file or directory\n");
                    return -1;
                }
                
                i++; // Skip the filename token in next iteration
            } else {
                printf("Error: Missing filename after input redirection\n");
                if (input_fd != -1) {
                    close(input_fd);
                }
                return -1;
            }
        }
    }
    
    // If input redirection was found, set it up
    if (input_fd != -1) {
        // Redirect standard input to the file
        if (dup2(input_fd, STDIN_FILENO) == -1) {
            perror("dup2 input redirection");
            close(input_fd);
            return -1;
        }
        
        // Close the original file descriptor to avoid leaks
        close(input_fd);
    }
    
    return 0;
}

/**
 * @brief Check if input redirection is present in the command
 * @param start_index Starting index in the tokens array
 * @param end_index Ending index in the tokens array
 * @return 1 if input redirection is present, 0 otherwise
 */
int has_input_redirection(int start_index, int end_index) {
    for (int i = start_index; i < end_index; i++) {
        if (tokens[i].type == TOKEN_INPUT) {
            return 1;
        }
    }
    return 0;
}

/**
 * @brief Get the input filename from redirection tokens
 * @param start_index Starting index in the tokens array
 * @param end_index Ending index in the tokens array
 * @return Pointer to the filename string, or NULL if not found
 */
char* get_input_filename(int start_index, int end_index) {
    char *input_filename = NULL;
    
    // Find the last input redirection (in case of multiple)
    for (int i = start_index; i < end_index; i++) {
        if (tokens[i].type == TOKEN_INPUT) {
            if (i + 1 < end_index && tokens[i + 1].type == TOKEN_NAME) {
                input_filename = tokens[i + 1].value;
            }
        }
    }
    
    return input_filename;
}