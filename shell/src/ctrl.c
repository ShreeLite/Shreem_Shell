#include "shell.h"
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sys/types.h>
#include <errno.h>

// Global variable to track current foreground process group
pid_t current_foreground_pgid = 0;
char current_foreground_command[MAX_PATH_LEN] = "";

/**
 * @brief Signal handler for SIGINT (Ctrl-C)
 * 
 * Requirements implemented:
 * - Send SIGINT to current foreground child process group if one exists
 * - Shell itself must not terminate on Ctrl-C
 */
void sigint_handler(int sig) {
    (void)sig; // Suppress unused parameter warning
    
    if (current_foreground_pgid > 0) {
        // Send SIGINT to the foreground process group
        kill(-current_foreground_pgid, SIGINT);
    }
    
    // Print newline and prompt to maintain shell responsiveness
    printf("\n");
    fflush(stdout);
}

/**
 * @brief Signal handler for SIGTSTP (Ctrl-Z)
 * 
 * Requirements implemented:
 * - Send SIGTSTP to current foreground child process group if one exists
 * - Move stopped process to background list with status "Stopped"
 * - Print: [job_number] Stopped command_name
 * - Shell itself must not stop on Ctrl-Z
 */
void sigtstp_handler(int sig) {
    (void)sig; // Suppress unused parameter warning
    
    if (current_foreground_pgid > 0) {
        // Send SIGTSTP to the foreground process group
        kill(-current_foreground_pgid, SIGTSTP);
        
        // Add the stopped process to background jobs (without launch message)
        int job_number = add_stopped_job(current_foreground_pgid, current_foreground_command);
        
        if (job_number > 0) {
            // Extract command name for display
            char command_name[MAX_PATH_LEN];
            strncpy(command_name, current_foreground_command, MAX_PATH_LEN - 1);
            command_name[MAX_PATH_LEN - 1] = '\0';
            char *space = strchr(command_name, ' ');
            if (space) {
                *space = '\0';
            }
            
            printf("\n[%d] Stopped %s\n", job_number, command_name);
        }
        
        // Clear foreground process tracking
        current_foreground_pgid = 0;
        current_foreground_command[0] = '\0';
    } else {
        printf("\n");
    }
    
    fflush(stdout);
}

/**
 * @brief Setup signal handlers for job control
 * 
 * Requirements implemented:
 * - Install signal handler for SIGINT (Ctrl-C)
 * - Install signal handler for SIGTSTP (Ctrl-Z)
 */
void setup_signal_handlers(void) {
    struct sigaction sa_int, sa_tstp;
    
    // Setup SIGINT handler (Ctrl-C)
    sa_int.sa_handler = sigint_handler;
    sigemptyset(&sa_int.sa_mask);
    sa_int.sa_flags = SA_RESTART;
    if (sigaction(SIGINT, &sa_int, NULL) == -1) {
        perror("sigaction SIGINT");
    }
    
    // Setup SIGTSTP handler (Ctrl-Z)
    sa_tstp.sa_handler = sigtstp_handler;
    sigemptyset(&sa_tstp.sa_mask);
    sa_tstp.sa_flags = SA_RESTART;
    if (sigaction(SIGTSTP, &sa_tstp, NULL) == -1) {
        perror("sigaction SIGTSTP");
    }
}

/**
 * @brief Set the current foreground process for signal handling
 * @param pgid Process group ID of the foreground process
 * @param command Command string for display purposes
 */
void set_foreground_process(pid_t pgid, const char* command) {
    current_foreground_pgid = pgid;
    if (command) {
        strncpy(current_foreground_command, command, MAX_PATH_LEN - 1);
        current_foreground_command[MAX_PATH_LEN - 1] = '\0';
    } else {
        current_foreground_command[0] = '\0';
    }
}

/**
 * @brief Clear the current foreground process tracking
 */
void clear_foreground_process(void) {
    current_foreground_pgid = 0;
    current_foreground_command[0] = '\0';
}

/**
 * @brief Handle Ctrl-D (EOF) condition
 * 
 * Requirements implemented:
 * - Detect EOF condition
 * - Send SIGKILL to all child processes
 * - Exit with status 0
 * - Print "logout" before exiting
 */
void handle_eof(void) {
    printf("logout\n");
    
    // Send SIGKILL to all background processes
    for (int i = 0; i < MAX_BACKGROUND_JOBS; i++) {
        if (background_jobs[i].is_active) {
            kill(background_jobs[i].pid, SIGKILL);
        }
    }
    
    // Exit with status 0
    exit(0);
}