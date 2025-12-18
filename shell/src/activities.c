#include "shell.h"
#include <string.h>
#include <stdlib.h>

/**
 * @brief Structure for sorting activities output
 */
typedef struct {
    pid_t pid;
    char command_name[MAX_PATH_LEN];
    ProcessState state;
} ActivityInfo;

/**
 * @brief Comparison function for qsort to sort activities by command name
 * @param a First activity info
 * @param b Second activity info
 * @return Comparison result for lexicographic ordering
 */
int compare_activities(const void *a, const void *b) {
    const ActivityInfo *activity_a = (const ActivityInfo *)a;
    const ActivityInfo *activity_b = (const ActivityInfo *)b;
    
    return strcmp(activity_a->command_name, activity_b->command_name);
}

/**
 * @brief Execute the activities command
 * 
 * Requirements implemented:
 * - Display format: [pid] : command_name - State
 * - Sort output lexicographically by command name
 * - Remove terminated processes from list (done in check_background_jobs)
 * - Show "Running" for running processes and "Stopped" for stopped processes
 */
void execute_activities(void) {
    ActivityInfo activities[MAX_BACKGROUND_JOBS];
    int activity_count = 0;
    
    // Collect all active background jobs
    for (int i = 0; i < MAX_BACKGROUND_JOBS; i++) {
        if (background_jobs[i].is_active) {
            activities[activity_count].pid = background_jobs[i].pid;
            strncpy(activities[activity_count].command_name, 
                   background_jobs[i].command_name, 
                   MAX_PATH_LEN - 1);
            activities[activity_count].command_name[MAX_PATH_LEN - 1] = '\0';
            activities[activity_count].state = background_jobs[i].state;
            activity_count++;
        }
    }
    
    // Sort activities lexicographically by command name
    if (activity_count > 0) {
        qsort(activities, activity_count, sizeof(ActivityInfo), compare_activities);
        
        // Display sorted activities
        for (int i = 0; i < activity_count; i++) {
            const char *state_str = (activities[i].state == PROCESS_RUNNING) ? "Running" : "Stopped";
            printf("[%d] : %s - %s\n", 
                   activities[i].pid, 
                   activities[i].command_name, 
                   state_str);
        }
    }
    // If no active processes, print nothing (as per requirement)
}