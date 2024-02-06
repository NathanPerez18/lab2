#include "utility.h"

#define MAX_INPUT_LENGTH 512
#define MAX_PATH_LENGTH 1024

void set_shell_env_variable() { //* Requirement 1. IX
    char execpath[MAX_PATH_LENGTH];

    // Read the symbolic link '/proc/self/exe' to get the path of the current executable
    // Using ssize_t as the readlink can return -1 (negative) therefore signed int is needd
    
    ssize_t len = readlink("/proc/self/exe", execpath, sizeof(execpath) - 1);
    
    if (len != -1) {
        execpath[len] = '\0'; // Null-terminate the path
        
        if (setenv("shell", execpath, 1) != 0) { // Set the 'shell' environment variable
            perror("setenv failure !!");
        } else {
            printf("\nShell environment variable set to: %s\n", execpath);
        }
    } else {
        perror("readlink failure !!");
    }
}
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// Include other necessary headers and declarations

int main(int argc, char *argv[]) {
    
    set_shell_env_variable();

    char* SHELL_PREFIX = "> myshell";
    char* SHELL_SEPERATOR = "$"; 

    char* GREEN_EC =  "\033[1;32m";
    char* YELLOW_EC = "\033[1;33m";
    char* RESET_EC =  "\033[0m";

    char input[MAX_INPUT_LENGTH];
    char cwd[MAX_PATH_LENGTH];
    char full_prompt[MAX_INPUT_LENGTH + MAX_PATH_LENGTH];

    FILE *input_stream = stdin; // Default to standard input

    // Check for batch mode
    if (argc > 1) {
        input_stream = fopen(argv[1], "r");
        if (input_stream == NULL) {
            perror("Error opening file");
            return 1;
        }
    }

    printf("Welcome to myshell, By: Nathan Perez & Logan Butler \n\n");

    // Main loop
    while (1) {

        if (input_stream == stdin) { // Only print prompt in interactive mode
            if (getcwd(cwd, sizeof(cwd)) == NULL) {
                perror("Error getting current directory");
                continue;
            }

            snprintf(full_prompt, sizeof(full_prompt), "%s%s:~%s%s%s%s  ", 
            YELLOW_EC, SHELL_PREFIX, GREEN_EC, cwd, RESET_EC, SHELL_SEPERATOR);

            printf("%s", full_prompt);
            fflush(stdout); 
        }
        
        if (fgets(input, MAX_INPUT_LENGTH, input_stream) == NULL) {
            if (input_stream != stdin) {
                fclose(input_stream);
            }
            break; // Exit if input fails (EOF or error)
        }

        input[strcspn(input, "\n")] = 0;

        command_pipeline(input);

        if (strcmp(input, "exit") == 0) {
            break;
        }
    }

    if (input_stream != stdin) {
        fclose(input_stream); // Close the file if in batch mode
    }

    return 0;
}
