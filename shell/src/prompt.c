#include "shell.h"

void prompt(const char*home_directory)
{   //Getting username using user id and passwd struct
    struct passwd *user=getpwuid(geteuid());
    const char*username=user ?user->pw_name:"user";
    //printf("%s",username);


    char hostname[1024];
    //Getting hostname
    if(gethostname(hostname,sizeof(hostname))!=0)
    {
        strcpy(hostname,"system");
    }
    //printf("%s",hostname);

    //Getting current working directory
    char cwd[1024];
    if(getcwd(cwd,sizeof(cwd))==NULL)
    {
          perror("cwd failed");
          strcpy(cwd,"?");
    }
   // printf("%s",cwd);

    //Home path and tilde logic
    char path[1024];
    size_t home_path_length = strlen(home_directory);
    
    // Check if current directory is exactly the home directory or a subdirectory of it
    if (strncmp(home_directory, cwd, home_path_length) == 0 && 
        (cwd[home_path_length] == '/' || cwd[home_path_length] == '\0')) {
        
        if (cwd[home_path_length] == '\0') {
            // Current directory is exactly the home directory
            strcpy(path, "~");
        } else {
            // Current directory is a subdirectory of home
            path[0] = '~';
            strcpy(path + 1, cwd + home_path_length);
        }
    } else {
        // Current directory is not under home directory
        strcpy(path, cwd);
    }

    //Printing the final prompt
    printf("<%s@%s:%s> ",username,hostname,path);
    fflush(stdout);
}