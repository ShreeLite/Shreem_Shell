#include "shell.h"
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <ctype.h>
//LLM GENERATED CODE BEGINS HERE
/**
 * @brief Check if a string represents a valid positive integer
 * @param str String to check
 * @return 1 if valid positive integer, 0 otherwise
 */
int is_valid_job_number(const char *str) {
    if (str == NULL || *str == '\0') {
        return 0;
    }
    
    // Check if all characters are digits
    for (int i = 0; str[i] != '\0'; i++) {
        if (!isdigit(str[i])) {
            return 0;
        }
    }
    
    return 1;
}

/**
 * @brief Find the most recently created background/stopped job
 * @return Index of most recent job, or -1 if none found
 */
int find_most_recent_job(void) {
    int most_recent = -1;
    int highest_job_number = 0;
    
    for (int i = 0; i < MAX_BACKGROUND_JOBS; i++) {
        if (background_jobs[i].is_active && 
            background_jobs[i].job_number > highest_job_number) {
            highest_job_number = background_jobs[i].job_number;
            most_recent = i;
        }
    }
    
    return most_recent;
}

/**
 * @brief Find background job by job number
 * @param job_number Job number to search for
 * @return Index of job in background_jobs array, or -1 if not found
 */
int find_job_by_number(int job_number) {
    for (int i = 0; i < MAX_BACKGROUND_JOBS; i++) {
        if (background_jobs[i].is_active && 
            background_jobs[i].job_number == job_number) {
            return i;
        }
    }
    return -1;
}

/**
 * @brief Execute the fg command
 * 
 * Requirements implemented:
 * - Bring background or stopped job to foreground
 * - Send SIGCONT to stopped jobs
 * - Wait for job to complete or stop again
 * - Use most recent job if no number provided
 * - Print "No such job" for invalid job numbers
 * - Print entire command when bringing to foreground
 */
void execute_fg(void) {
    int job_index = -1;
    
    // Check arguments
    if (token_count == 1) {
        // No job number provided - use most recent job
        job_index = find_most_recent_job();
        if (job_index == -1) {
            printf("No such job\n");
            return;
        }
    } else if (token_count == 2) {
        // Job number provided
        if (tokens[1].type != TOKEN_NAME || !is_valid_job_number(tokens[1].value)) {
            printf("No such job\n");
            return;
        }
        
        int job_number = atoi(tokens[1].value);
        job_index = find_job_by_number(job_number);
        if (job_index == -1) {
            printf("No such job\n");
            return;
        }
    } else {
        printf("No such job\n");
        return;
    }
    
    // Get the job
    BackgroundJob *job = &background_jobs[job_index];
    
    // Print the command being brought to foreground
    printf("%s\n", job->command);
    
    // If job is stopped, send SIGCONT to resume it
    if (job->state == PROCESS_STOPPED) {
        if (kill(-job->pid, SIGCONT) == -1) {
            perror("kill SIGCONT");
            return;
        }
    }
    
    // Set as foreground process
    set_foreground_process(job->pid, job->command);
    
    // Wait for the job to complete or stop
    int status;
    pid_t result = waitpid(job->pid, &status, WUNTRACED);
    
    if (result == -1) {
        perror("waitpid");
        clear_foreground_process();
        return;
    }
    
    // Clear foreground tracking
    clear_foreground_process();
    
    if (WIFEXITED(status) || WIFSIGNALED(status)) {
        // Job terminated - remove from background jobs
        cleanup_background_job(job_index);
    } else if (WIFSTOPPED(status)) {
        // Job was stopped again - update state
        job->state = PROCESS_STOPPED;
        printf("\n[%d] Stopped %s\n", job->job_number, job->command_name);
    }
}

/**
 * @brief Execute the bg command
 * 
 * Requirements implemented:
 * - Resume stopped background job by sending SIGCONT
 * - Job continues running in background
 * - Print [job_number] command_name & when resuming
 * - Print "Job already running" for running jobs
 * - Print "No such job" for invalid job numbers
 * - Only stopped jobs can be resumed
 */
void execute_bg(void) {
    int job_index = -1;
    
    // Check arguments
    if (token_count == 1) {
        // No job number provided - use most recent job
        job_index = find_most_recent_job();
        if (job_index == -1) {
            printf("No such job\n");
            return;
        }
    } else if (token_count == 2) {
        // Job number provided
        if (tokens[1].type != TOKEN_NAME || !is_valid_job_number(tokens[1].value)) {
            printf("No such job\n");
            return;
        }
        
        int job_number = atoi(tokens[1].value);
        job_index = find_job_by_number(job_number);
        if (job_index == -1) {
            printf("No such job\n");
            return;
        }
    } else {
        printf("No such job\n");
        return;
    }
    
    // Get the job
    BackgroundJob *job = &background_jobs[job_index];
    
    // Check if job is already running
    if (job->state == PROCESS_RUNNING) {
        printf("Job already running\n");
        return;
    }
    
    // Job must be stopped to be resumed
    if (job->state == PROCESS_STOPPED) {
        // Send SIGCONT to resume the job
        if (kill(-job->pid, SIGCONT) == -1) {
            perror("kill SIGCONT");
            return;
        }
        
        // Update state to running
        job->state = PROCESS_RUNNING;
        
        // Print resumption message
        printf("[%d] %s &\n", job->job_number, job->command_name);
    } else {
        printf("No such job\n");
    }
}

//LLM GENERATED CODE ENDS HERE