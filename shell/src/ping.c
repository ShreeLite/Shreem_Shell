#include "shell.h"
#include <signal.h>
#include <errno.h>
#include <stdlib.h>
#include <ctype.h>

/**
 * @brief Check if a string represents a valid integer
 * @param str String to check
 * @return 1 if valid integer, 0 otherwise
 */
int is_valid_number(const char *str) {
    if (str == NULL || *str == '\0') {
        return 0;
    }
    
    // Allow negative numbers (start with -)
    int i = 0;
    if (str[0] == '-') {
        i = 1;
        if (str[1] == '\0') {
            return 0; // Just a minus sign
        }
    }
    
    // Check if all remaining characters are digits
    for (; str[i] != '\0'; i++) {
        if (!isdigit(str[i])) {
            return 0;
        }
    }
    
    return 1;
}

/**
 * @brief Execute the ping command
 * 
 * Requirements implemented:
 * - Syntax: ping <pid> <signal_number>
 * - Signal modulo 32: actual_signal = signal_number % 32
 * - Error handling for non-existent processes
 * - Success message on signal delivery
 * - Invalid syntax for non-numeric signal_number
 */
void execute_ping(void) {
    // Check for correct number of arguments
    // tokens[0] = "ping", tokens[1] = <pid>, tokens[2] = <signal_number>
    if (token_count < 3) {
        printf("Invalid syntax!\n");
        return;
    }
    
    if (token_count > 3) {
        printf("Invalid syntax!\n");
        return;
    }
    
    // Validate that both arguments are present and are names (not operators)
    if (tokens[1].type != TOKEN_NAME || tokens[2].type != TOKEN_NAME) {
        printf("Invalid syntax!\n");
        return;
    }
    
    // Parse PID
    if (!is_valid_number(tokens[1].value)) {
        printf("Invalid syntax!\n");
        return;
    }
    
    pid_t target_pid = (pid_t)atoi(tokens[1].value);
    
    // Parse signal number
    if (!is_valid_number(tokens[2].value)) {
        printf("Invalid syntax!\n");
        return;
    }
    
    int signal_number = atoi(tokens[2].value);
    
    // Apply modulo 32 as required
    int actual_signal = signal_number % 32;
    
    // Attempt to send the signal
    int result = kill(target_pid, actual_signal);
    
    if (result == -1) {
        // Check the specific error
        if (errno == ESRCH) {
            printf("No such process found\n");
        } else {
            // Other errors (like permission denied)
            printf("No such process found\n");
        }
    } else {
        // Success
        printf("Sent signal %d to process with pid %d\n", signal_number, target_pid);
    }
}