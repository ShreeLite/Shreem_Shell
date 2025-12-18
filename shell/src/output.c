#include "shell.h"
#include <fcntl.h>
#include <unistd.h>

/**
 * @brief Set up output redirection for a command
 * @param start_index Starting index in the tokens array
 * @param end_index Ending index in the tokens array
 * @return 0 on success, -1 on error
 * 
 * Requirements implemented:
 * - Supports both > (overwrite) and >> (append) redirection
 * - Uses open() with appropriate flags (O_WRONLY | O_CREAT | O_TRUNC/O_APPEND)
 * - Uses dup2() to redirect STDOUT_FILENO
 * - Closes original file descriptor after duplication
 * - Handles multiple output redirections (last one takes effect)
 * - Sets file permissions to 0644 for created files
 */
int setup_output_redirection(int start_index, int end_index) {
    char *output_filename = NULL;
    int output_fd = -1;
    int output_flags = 0;
    
    // Scan through tokens to find output redirection
    // If multiple output redirections are present, only the last one takes effect
    for (int i = start_index; i < end_index; i++) {
        if (tokens[i].type == TOKEN_OUTPUT || tokens[i].type == TOKEN_DOUBLE_OUTPUT) {
            // Check if there's a filename after the redirection token
            if (i + 1 < end_index && tokens[i + 1].type == TOKEN_NAME) {
                // Close previous output file descriptor if it exists
                if (output_fd != -1) {
                    close(output_fd);
                }
                
                output_filename = tokens[i + 1].value;
                
                // Set flags based on redirection type
                if (tokens[i].type == TOKEN_OUTPUT) {
                    // Overwrite mode: truncate if file exists, create if it doesn't
                    output_flags = O_WRONLY | O_CREAT | O_TRUNC;
                } else { // TOKEN_DOUBLE_OUTPUT
                    // Append mode: append to file if exists, create if it doesn't
                    output_flags = O_WRONLY | O_CREAT | O_APPEND;
                }
                
                // Open the output file with appropriate flags
                output_fd = open(output_filename, output_flags, 0644);
                
                if (output_fd == -1) {
                    printf("Unable to create file for writing\n");
                    return -1;
                }
                
                i++; // Skip the filename token in next iteration
            } else {
                printf("Error: Missing filename after output redirection\n");
                if (output_fd != -1) {
                    close(output_fd);
                }
                return -1;
            }
        }
    }
    
    // If output redirection was found, set it up
    if (output_fd != -1) {
        // Redirect standard output to the file
        if (dup2(output_fd, STDOUT_FILENO) == -1) {
            perror("dup2 output redirection");
            close(output_fd);
            return -1;
        }
        
        // Close the original file descriptor to avoid leaks
        close(output_fd);
    }
    
    return 0;
}

/**
 * @brief Check if output redirection is present in the command
 * @param start_index Starting index in the tokens array
 * @param end_index Ending index in the tokens array
 * @return 1 if output redirection is present, 0 otherwise
 */
int has_output_redirection(int start_index, int end_index) {
    for (int i = start_index; i < end_index; i++) {
        if (tokens[i].type == TOKEN_OUTPUT || tokens[i].type == TOKEN_DOUBLE_OUTPUT) {
            return 1;
        }
    }
    return 0;
}

/**
 * @brief Get the output filename from redirection tokens
 * @param start_index Starting index in the tokens array
 * @param end_index Ending index in the tokens array
 * @return Pointer to the filename string, or NULL if not found
 */
char* get_output_filename(int start_index, int end_index) {
    char *output_filename = NULL;
    
    // Find the last output redirection (in case of multiple)
    for (int i = start_index; i < end_index; i++) {
        if (tokens[i].type == TOKEN_OUTPUT || tokens[i].type == TOKEN_DOUBLE_OUTPUT) {
            if (i + 1 < end_index && tokens[i + 1].type == TOKEN_NAME) {
                output_filename = tokens[i + 1].value;
            }
        }
    }
    
    return output_filename;
}

/**
 * @brief Get the output redirection type
 * @param start_index Starting index in the tokens array
 * @param end_index Ending index in the tokens array
 * @return TOKEN_OUTPUT for >, TOKEN_DOUBLE_OUTPUT for >>, or -1 if not found
 */
int get_output_type(int start_index, int end_index) {
    int output_type = -1;
    
    // Find the last output redirection type (in case of multiple)
    for (int i = start_index; i < end_index; i++) {
        if (tokens[i].type == TOKEN_OUTPUT || tokens[i].type == TOKEN_DOUBLE_OUTPUT) {
            output_type = tokens[i].type;
        }
    }
    
    return output_type;
}