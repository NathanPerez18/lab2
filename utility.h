
#ifndef UTILITY_H
#define UTILITY_H


// Core Generic Imports

#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

//Environment & Forking Libs

#include <dirent.h>
#include <sys/wait.h>
#include <sys/wait.h>
#include <sys/types.h>

#define MAX_TOKENS 100
#define MAX_TOKEN_LENGTH 50

typedef enum {
    KEYWORD, //int 0
    LITERAL, //int 1
    OPERATOR //int 2
} TokenType;

typedef struct Token Token; // Forward Declaration is need for Circular Dependency with CommandFunc void fn. ptr

typedef void (*CommandFunc)(const Token* tokens, int token_count);

struct Token {
    char* token;
    TokenType type;
    CommandFunc func; //* Function Pointer for this Token's Keyword or Operator
};

//* Internal Command ; Declarations
void change_directory(const Token* tokens, int token_count);
void clear_screen(const Token* tokens, int token_count);
void list_dir(const Token* tokens, int token_count);
void environ_func(const Token* tokens, int token_count);
void echo(const Token* tokens, int token_count);
void help(const Token* tokens, int token_count);
void pause_shell(const Token* tokens, int token_count);
void quit(const Token* tokens, int token_count);

int execute_external_command(const Token* tokens, int token_count);

//* I/O Redirection & Utility Declarations

extern char* STDOUT_REDIR_BUFFER;
extern char* OUTPUT_WRITE_MODE; // "w" for Re-write, and "a" for Append

void input_redir(const Token* tokens, int token_count);

// Command Parsing Pipeline
void command_pipeline(char* input) ;

char** parse_and_clean(const char* input, int* numTokens);
TokenType determine_token_type(const char* token, CommandFunc* func);
void tokenize(char** tokens, int numTokens, Token* tokenStructs);
void process_tokens(Token* tokens, int numTokens);

#endif 
