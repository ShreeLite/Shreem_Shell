#include "shell.h"
#include <dirent.h>
#include <sys/stat.h>

/**
 * @brief Compare function for sorting directory entries lexicographically (case-insensitive)
 */
int compare_entries(const void *a, const void *b) {
    const char **str_a = (const char **)a;
    const char **str_b = (const char **)b;
    return strcasecmp(*str_a, *str_b);
}

/**
 * @brief Parse reveal flags from tokens
 * @param show_hidden Pointer to boolean for -a flag
 * @param line_format Pointer to boolean for -l flag
 * @return Starting index of non-flag arguments
 */
int parse_reveal_flags(bool *show_hidden, bool *line_format) {
    *show_hidden = false;
    *line_format = false;
    
    int i = 1; // Start from first argument after "reveal"
    
    while (i < token_count && tokens[i].value[0] == '-') {
        char *flag_str = tokens[i].value;
        
        // Special case: "-" should not be treated as a flag
        if (strcmp(flag_str, "-") == 0) {
            break;
        }
        
        // Process each character in the flag string
        for (int j = 1; flag_str[j] != '\0'; j++) {
            if (flag_str[j] == 'a') {
                *show_hidden = true;
            } else if (flag_str[j] == 'l') {
                *line_format = true;
            }
            // Ignore other characters (as per example behavior)
        }
        i++;
    }
    
    return i; // Return index of first non-flag argument
}

/**
 * @brief Get the target directory path for reveal command
 * @param home_directory The shell's home directory
 * @param arg_index Index of the directory argument in tokens
 * @param target_path Buffer to store the resolved path
 * @return 0 on success, -1 on error
 */
int get_reveal_target_path(const char* home_directory, int arg_index, char *target_path) {
    // Find the first directory argument (skip redirection tokens)
    char *arg = NULL;
    for (int i = arg_index; i < token_count; i++) {
        if (tokens[i].type == TOKEN_NAME) {
            // Check if this is a redirection filename
            if (i > 0 && (tokens[i-1].type == TOKEN_OUTPUT || 
                         tokens[i-1].type == TOKEN_DOUBLE_OUTPUT || 
                         tokens[i-1].type == TOKEN_INPUT)) {
                continue; // Skip redirection filenames
            }
            arg = tokens[i].value;
            break;
        }
    }
    
    if (arg == NULL) {
        // No directory argument, use current directory
        if (getcwd(target_path, MAX_PATH_LEN) == NULL) {
            perror("getcwd error");
            return -1;
        }
        return 0;
    }
    
    if (strcmp(arg, "~") == 0) {
        strcpy(target_path, home_directory);
    } else if (strcmp(arg, ".") == 0) {
        if (getcwd(target_path, MAX_PATH_LEN) == NULL) {
            perror("getcwd error");
            return -1;
        }
    } else if (strcmp(arg, "..") == 0) {
        if (getcwd(target_path, MAX_PATH_LEN) == NULL) {
            perror("getcwd error");
            return -1;
        }
        // Get parent directory
        char *last_slash = strrchr(target_path, '/');
        if (last_slash != NULL && last_slash != target_path) {
            *last_slash = '\0';
        } else if (last_slash == target_path) {
            strcpy(target_path, "/");
        }
    } else if (strcmp(arg, "-") == 0) {
        if (strlen(previous_cwd) == 0) {
            // No previous directory set
            printf("No such directory!\n");
            return -1;
        } else {
            strcpy(target_path, previous_cwd);
        }
    } else {
        // It's a regular path (absolute or relative)
        if (arg[0] == '/') {
            // Absolute path
            strcpy(target_path, arg);
        } else {
            // Relative path
            if (getcwd(target_path, MAX_PATH_LEN) == NULL) {
                perror("getcwd error");
                return -1;
            }
            strcat(target_path, "/");
            strcat(target_path, arg);
        }
    }
    
    return 0;
}

/**
 * @brief Execute the reveal command
 * @param home_directory The shell's home directory
 */
void execute_reveal(const char* home_directory) {
    bool show_hidden = false;
    bool line_format = false;
    
    // Parse flags
    int arg_index = parse_reveal_flags(&show_hidden, &line_format);
    
    // Check for too many arguments - should have at most one directory argument
    int dir_args = 0;
    for (int i = arg_index; i < token_count; i++) {
        if (tokens[i].type == TOKEN_NAME) {
            // Check if this is a redirection filename
            if (i > 0 && (tokens[i-1].type == TOKEN_OUTPUT || 
                         tokens[i-1].type == TOKEN_DOUBLE_OUTPUT || 
                         tokens[i-1].type == TOKEN_INPUT)) {
                continue; // Skip redirection filenames
            }
            dir_args++;
        }
    }
    if (dir_args > 1) {
        printf("reveal: Invalid Syntax!\n");
        return;
    }
    
    // Get target directory path
    char target_path[MAX_PATH_LEN];
    if (get_reveal_target_path(home_directory, arg_index, target_path) != 0) {
        return;
    }
    
    // Open directory
    DIR *dir = opendir(target_path);
    if (dir == NULL) {
        printf("No such directory!\n");
        return;
    }
    
    // Read directory entries
    struct dirent *entry;
    char **entries = malloc(1000 * sizeof(char*)); // Allocate for up to 1000 entries
    int entry_count = 0;
    
    while ((entry = readdir(dir)) != NULL) {
        // Skip hidden files if -a flag is not set
        if (!show_hidden && entry->d_name[0] == '.') {
            continue;
        }
        
        // Allocate memory for entry name and copy it
        entries[entry_count] = malloc(strlen(entry->d_name) + 1);
        strcpy(entries[entry_count], entry->d_name);
        entry_count++;
    }
    
    closedir(dir);
    
    // Sort entries lexicographically
    qsort(entries, entry_count, sizeof(char*), compare_entries);
    
    // Display entries
    if (line_format) {
        // One entry per line
        for (int i = 0; i < entry_count; i++) {
            printf("%s\n", entries[i]);
        }
    } else {
        // Space-separated format (like ls)
        for (int i = 0; i < entry_count; i++) {
            printf("%s", entries[i]);
            if (i < entry_count - 1) {
                printf(" ");
            }
        }
        if (entry_count > 0) {
            printf("\n");
        }
    }
    
    // Free allocated memory
    for (int i = 0; i < entry_count; i++) {
        free(entries[i]);
    }
    free(entries);
}