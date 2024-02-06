#include "utility.h"

char* STDOUT_REDIR_BUFFER = NULL;
char* OUTPUT_WRITE_MODE = "w";

//! ------------------ Internal Command Implementations -------------------------

void change_directory(const Token* tokens, int token_count) {
    if (token_count > 1) {
        printf("Attempting to change directory to: '%s'\n", tokens[1].token);
        if (chdir(tokens[1].token) != 0) {
            perror("Change directory failed");
        } else {
            // Successfully changed directory
            char cwd[1024];
            if (getcwd(cwd, sizeof(cwd)) != NULL) {
                printf("New directory: %s\n", cwd);
            } else {
                perror("Error getting current directory");
            }
        }
    } else {
        printf("No directory specified for cd command\n");
    }
}



void clear_screen(const Token* tokens, int token_count) { //1.ii
    printf("\033[H\033[J");
}

void list_dir(const Token* tokens, int token_count) { //1.iii
    const char* directory = (token_count > 0) ? tokens[0].token : ".";

    DIR *d;
    struct dirent *dir;
    d = opendir(directory);
    if (d) {
        while ((dir = readdir(d)) != NULL) {
            printf("%s\n", dir->d_name);
        }
        closedir(d);
    } else {
        perror("dir command failed");
    }
}

extern char **environ; //* Requires the external environ variable declared in <unistd.h>.

void environ_func(const Token* tokens, int token_count) { //1.iv
    for (char **env = environ; *env != 0; env++) {
        char *thisEnv = *env;
        printf("%s\n", thisEnv);    
    }
}

void echo(const Token* tokens, int token_count) { //1.v
    for (int i = 0; i < token_count; i++) {
        printf("%s ", tokens[i].token);
    }
    printf("\n");
}

void help(const Token* tokens, int token_count) { //1.vi
    system("more manual.txt"); //TODO Replace 'manual.txt' with the manual file
}

void pause_shell(const Token* tokens, int token_count) { //1.vii
    printf("Press Enter to continue...");
    getchar();
}

void quit(const Token* tokens, int token_count) { // 1.viii
    exit(0);
}


//! --req 2 

int execute_external_command(const Token* tokens, int token_count) {
    if (token_count == 0) {
        return 1; // No command to execute
    }

    char shell_path[1024];
    if (readlink("/proc/self/exe", shell_path, sizeof(shell_path) - 1) == -1) {
        perror("readlink failed to get shell path");
        return 1;
    }

    pid_t pid = fork();
    if (pid == -1) {
        perror("Fork failed");
        return 1; // Fork failure
    } else if (pid == 0) { // Child process
        setenv("parent", shell_path, 1); // Set parent environment variable

        // Prepare arguments for execvp
        char* args[token_count + 1];
        for (int i = 0; i < token_count; i++) {
            args[i] = tokens[i].token;
        }
        args[token_count] = NULL; // Null-terminate the argument list

        // Execute the command
        if (execvp(args[0], args) == -1) {
            perror("Execution of program failed");
            exit(EXIT_FAILURE); // Exit the child process
        }
    } else { // Parent process
        int status;
        waitpid(pid, &status, 0); // Wait for the child process to finish

        if (WIFEXITED(status)) {
            printf("Child exited with status %d\n", WEXITSTATUS(status));
        } else if (WIFSIGNALED(status)) {
            printf("Child terminated by signal %d\n", WTERMSIG(status));
        }
    }

    return 0;
}

//! ------------------ Input & Output Redirection Functionality ------------------

void input_redir(const Token* tokens, int token_count) {
    
    if (token_count > 0 && tokens[0].token != NULL) {
        
        const char* filePath = tokens[0].token;

        // Check if the file exists in the current working directory
        if (access(tokens[0].token, F_OK) != 0) {
            perror("Failed to find the file for input redirection");
            return;
        }

        // Redirect stdin to read from the file
        if (freopen(filePath, "r", stdin) == NULL) {
            perror("Failed to redirect stdin from file");
            return;
        }

        printf("stdin is now redirected to read from '%s'\n", filePath);
    } else {
        fprintf(stderr, "No input file specified for redirection\n");
    }
}

void output_redir(const Token* tokens, int token_count) {
    if (token_count > 0 && tokens[0].token != NULL) {
        const char* filePath = tokens[0].token;

        // Open the file in append mode
        FILE* file = fopen(filePath, OUTPUT_WRITE_MODE);
        if (file == NULL) {
            perror("Failed to open file for appending");
            return;
        }

        // Append the contents of STDOUT_TEST_BUFFER to the file
        if (fputs(STDOUT_REDIR_BUFFER, file) == EOF) {
            perror("Failed to write to file");
        } else {
            printf("Contents appended to '%s'\n", filePath);
        }

        // Close the file
        fclose(file);
    } else {
        fprintf(stderr, "No output file specified for appending\n");
    }
}

//! ------------------ Parsing Functionality --------------------------

void command_pipeline(char* input) {
    int numTokens;
    char** tokens = parse_and_clean(input, &numTokens);

    Token* tokenStructs = (Token*)malloc(numTokens * sizeof(Token));
    tokenize(tokens, numTokens, tokenStructs);

    process_tokens(tokenStructs, numTokens);

    //Free Allocated Variables
    free(tokens);
    free(tokenStructs);
}

//* KEYWORD & OPERATOR token-definitions aswell as associated function Pointers (to be loaded into Token structs upon pipeline call)

const char* keywords[] = {"cd", "clr", "dir", "environ", "echo", "help", "pause", "quit"};
CommandFunc keywordFunctions[] = {change_directory, clear_screen, list_dir, environ_func, echo, help, pause_shell, quit};

const char* operators[] = {"<", ">", ">>"};
CommandFunc operatorFunctions[] = {input_redir, output_redir, output_redir};

//* In C, returning a char** (char ptr to ptr)
//* is a common way to represent a dynamically allocated array of strings (or array of pointers)

//TODO: comment btr
char** parse_and_clean(const char* input, int* numTokens) {
    char** tokens = (char**)malloc(MAX_TOKENS * sizeof(char*));
    char* token;
    char* tempInput = strdup(input); // Duplicate input string for strtok
    int i = 0;

    token = strtok(tempInput, " \t\n");
    while (token != NULL && i < MAX_TOKENS) {
        tokens[i] = strdup(token); // Duplicate token
        token = strtok(NULL, " \t\n");
        i++;
    }
    *numTokens = i;

    free(tempInput); //Free the copy of the Input (reducs mem. leaks)
    return tokens;
}

TokenType determine_token_type(const char* token, CommandFunc* func) {
    for (int i = 0; i < sizeof(keywords)/sizeof(char*); i++) {
        if (strcmp(token, keywords[i]) == 0) {
            *func = keywordFunctions[i];
            return KEYWORD;
        }
    }
    for (int i = 0; i < sizeof(operators)/sizeof(char*); i++) {
        if (strcmp(token, operators[i]) == 0) {
            *func = operatorFunctions[i];
            return OPERATOR;
        }
    }
    *func = NULL; // No function associated with literals
    return LITERAL;
}

//TODO: comment btr
void tokenize(char** tokens, int numTokens, Token* tokenStructs) {
    for (int i = 0; i < numTokens; i++) {
        
        CommandFunc func;
        tokenStructs[i].token = tokens[i];

        //* Pass the type determination fn, the `func` ptr to be set, before actually setting struct.func to func
        tokenStructs[i].type = determine_token_type(tokens[i], &func); 
        tokenStructs[i].func = func;
    }
}

void perform_output_redirection(Token* tokens, int numTokens) {
    
    //* (STDOUT REDIR) After execution of Command or Program, called in process_tokens
    // Our global STDOUT_REDIR_BUFFER might be filled with output, so now Output Redirect to files 
    // Either "Write" or "Append" modes
    
    for (int i = 0; i < numTokens; i++) {

        if(strcmp(tokens[i].token, ">") == 0 || strcmp(tokens[i].token, ">>") == 0){ //Inp. Redir. Operator
                
            OUTPUT_WRITE_MODE = "a";

            if(strcmp(tokens[i].token, ">") == 0)
                OUTPUT_WRITE_MODE = "w";
                
            tokens[i].func(&tokens[i+1], numTokens - 1);
            break;
        }
    }    
}


void process_tokens(Token* tokens, int numTokens) {
    
    //TODO: For debug, remove soon
    for (int i = 0; i < numTokens; i++) {
        printf("Token: %s, Type: %d\n", tokens[i].token, tokens[i-1].type);
    }

    // ------ Requirement 1. Internal Commands (Executing Associated Token Func.) ------
    
    if(tokens[0].type == KEYWORD && tokens[0].func != NULL){
        //* &tokens[1] points to tokens[1] and onwards (without traditional array slicing)
        tokens[0].func(&tokens[1], numTokens-1); // Only 1 Argument after Redirection operator
        return;
    }

    // ------ Requirement 2 & 4. External Program Invocation with Optional I/O Rediection ------
    
    if (tokens[0].type == LITERAL) { //* tk[0] indicates a program call
        
        
        // (STDIN) Before executing, let's first check if Input Redirection needs to be performed:
        for (int i = 0; i < numTokens; i++) {

            if(strcmp(tokens[i].token, "<") == 0){ //Inp. Redir. Operator check
                tokens[i].func(&tokens[i+1], 1); // Only 1 Argument after Redirection operator
                break;
            }
        }

        int operatorIndex = numTokens; // Default to numTokens if no operator is found

        // Find the first operator token, if any
        for (int i = 0; i < numTokens; i++) {
            if (tokens[i].type == OPERATOR) {
                operatorIndex = i;
                break;
            }
        }

        // Execute the command as an external program, passing only tokens before the first operator,    int stat = 
        execute_external_command(tokens, operatorIndex);

        // (STDOUT) Analyzes Tokenized inputs for Redirection Instructions, and performs accordingly
        perform_output_redirection(tokens,numTokens);
    }


}
