#include<stdlib.h>
#include<stdio.h>
#include<string.h>
#include <sys/wait.h>
#include <unistd.h>
#include <time.h>

#define BUFFER_SIZE 256
#define TRUE 1
#define FALSE 0

const char *builtin_commands[] = {"exit", "log", "print", "theme"};
int builtin_commands_count = 4;

typedef struct {
    char *name;
    time_t time;
    int exit_code;
} Command;

Command **commands;
int commands_array_max_size = 10;
int commands_array_curr_size = 0;

typedef struct {
    char *name;
    char *value;
} EnvVar;

EnvVar **env_variables;
int env_variables_array_max_size = 10;
int env_variables_array_curr_size = 0;



void remove_new_line(char *token) {
    char *new_line = strchr(token, '\n');
    if (new_line) {
        *new_line = 0;
    }
}

void remove_carriage_return(char *token) {
    char *carriage = strchr(token, '\r');
    if (carriage) {
        *carriage = 0;
    }
}

char *read_input() {

    // Read input from the user and remove the new line
    char *input = (char *)(malloc(sizeof(char) * BUFFER_SIZE));
    fgets(input, BUFFER_SIZE, stdin);
    remove_new_line(input);
    return input;
}

char **tokenize(char *input) {


    // Parse the input, seperating the token based on spaces and putting each token 
    // into an array of character pointers

    int max_size = 10;
    char **cmd_tokens = (char**)(malloc(sizeof(char*) * max_size));

    char delim[2] = " ";
    
    char *token = strtok(input, delim);
    int curr_size = 0;
    while (token != NULL) {

        cmd_tokens[curr_size] = token;
        curr_size++;

        if(curr_size >= max_size) {
            max_size *= 2;
            cmd_tokens = (char **)realloc(cmd_tokens, (sizeof(char*) * max_size));
        }
        token = strtok(NULL, delim);
    }

    cmd_tokens[curr_size] = '\0';

    return cmd_tokens;
}



int exec_cshell_exit(char **cmd_tokens) {

    // If an argument is passed, throw an error
    if(cmd_tokens[1]) {
        fprintf(stderr, "Error: Too many arguments passed\n");
        return 1;
    }

    // Print out a message and exit the program
    printf("Bye!\n");
    exit(0);
}

int exec_cshell_log(char **cmd_tokens) {

    // Check if there is something in the log history
    if(commands_array_curr_size == 0) {
        fprintf(stderr, "Error: Nothing to log\n");
        return 1;
    }

    // If an argument is passed, throw an error
    if(cmd_tokens[1]) {
        fprintf(stderr, "Error: Too many arguments passed\n");
        return 1;
    }

    // Print out the log history along with the time and exit code
    for (int i = 0; i < commands_array_curr_size; i++) {
        printf("%s", ctime(&commands[i]->time));
        printf("%s %d\n", commands[i]->name, commands[i]->exit_code);
    }

    return 0;
}

int exec_cshell_print(char **cmd_tokens) {

    if(cmd_tokens[1] == NULL) {
        fprintf(stderr, "Error: Expected at least one argument\n");
    }

    int i = 1;

    while(cmd_tokens[i]) {
        if(cmd_tokens[i][0] != '$') {
            // Normal print
            printf("%s ", cmd_tokens[i]);
        } else {
            // Print environmnet variables
            // Search for the particular environment variable and return its value
            int j;
            int flag = 0;
            for (j = 0; j < env_variables_array_curr_size; j++) {
                remove_new_line(env_variables[j]->name);
                if(strcmp(cmd_tokens[i], env_variables[j]->name) == 0) {
                    printf("%s ", env_variables[j]->value);
                    flag = 1;
                }
            }

            // If not found, throw an error indicating that the variable was not found
            if (j == env_variables_array_curr_size && flag == 0) {
                fprintf(stderr, "Error: No name and/or value found for setting up Environment Variables\n");
                return 1;
            }
        }
        i++;
    }
    printf("\n");

    return 0;
}

int exec_cshell_theme(char **cmd_tokens) {

    // Throw an error if more than one argument passed to theme
    if(cmd_tokens[2]) {
        fprintf(stderr, "Error: Too many arguments passed\n");
        return 1;
    }

    char *color = cmd_tokens[1];

    // Check if an argument was passed
    // If yes, set the theme based on that colour 
    // If color not part of the supported themes, throw an error
    if(color == NULL) {
        fprintf(stderr, "Error: No arguments passed\n");
        return 1;
    } else if (strcmp(color, "red") == 0) {
        printf("\033[0;31m");
    } else if (strcmp(color, "green") == 0) {
        printf("\033[0;32m");
    } else if (strcmp(color, "blue") == 0) {
        printf("\033[0;34m");
    } else {
        printf("unsupported theme\n");
        return 1;
    }

    return 0;
}

char *get_value(char *cmd_str) {

    int i;
    for (i = 0; i < strlen(cmd_str); i++) {
        if(cmd_str[i] == '=') {
            break;
        } else if (cmd_str[i] == ' ') {
            return NULL;
        }
    }

    if(cmd_str[i+1] == ' ' || cmd_str[i-1] == ' ') {
        fprintf(stderr, "Error: Wrong format for environment variables\n");
        return NULL;
    }

    char *value = cmd_str+i+1;

    return value;
}


char **parse_env_var(char *input) {

    // Parse the command, seperating the tokens based on "=" and store
    // it into an array of character pointers

    int max_size = 10;
    char **env_var_tokens = (char**)(malloc(sizeof(char*) * max_size));

    char cmd_str[BUFFER_SIZE];
    strcpy(cmd_str, input);

    char delim[2] = "=";
    env_var_tokens[0] = strtok(input, delim);
    env_var_tokens[1] = get_value(cmd_str);
    env_var_tokens[2] = '\0';

    return env_var_tokens;
}

int store_env_var(char **env_var_tokens) {

    // Return if wrong format
    if(env_var_tokens[1] == NULL || env_var_tokens[2]) {
        fprintf(stderr, "Error: Wrong format for Environment Variables\n");
        return 1;
    }

    // Dynamically increase the size of the array when needed
    if(env_variables_array_curr_size >= env_variables_array_max_size) {
        env_variables_array_max_size *= 2;
        env_variables = (EnvVar **)(realloc(env_variables, sizeof(EnvVar *) * env_variables_array_max_size));
    }

    // Check if the key/variable already exists in the array
    // If yes, simply update the value
    for (int i = 0; i < env_variables_array_curr_size; i++) {
        if(strcmp(env_variables[i]->name, env_var_tokens[0]) == 0) {
            strcpy(env_variables[i]->value, env_var_tokens[1]);
            return 0;
        }
    }


    // Create a new environment variable to store the key-value pairs 
    EnvVar *new_env_variable = (EnvVar *)(malloc(sizeof(EnvVar)));
    new_env_variable->name = (char *)(malloc(sizeof(char) * BUFFER_SIZE));
    strcpy(new_env_variable->name, env_var_tokens[0]);
    new_env_variable->value = (char *)(malloc(sizeof(char) * BUFFER_SIZE));
    strcpy(new_env_variable->value, env_var_tokens[1]);

    env_variables[env_variables_array_curr_size] = new_env_variable;
    env_variables_array_curr_size++;

    return 0;
}

void log_command(char *cmd_name, int exit_code) {

    // Dynamically increase the size of the array when needed
    if(commands_array_curr_size >= commands_array_max_size) {
        commands_array_max_size *= 2;
        commands = (Command **)(realloc(commands, sizeof(Command *) * commands_array_max_size));
    }

    // Make a new command that stores information about the command to be logged
    Command *new_command = (Command *)(malloc(sizeof(Command)));
    new_command->name = (char *)(malloc(sizeof(char) * BUFFER_SIZE));
    strcpy(new_command->name, cmd_name);
    new_command->time = time(NULL);
    new_command->exit_code = exit_code;

    commands[commands_array_curr_size] = new_command;
    commands_array_curr_size++;
}

void exec_builtin_command(char **cmd_tokens) {

    // Check for which command has the user inputted and
    // execute it. Log the commands after the execution

    if(strcmp(cmd_tokens[0], builtin_commands[0]) == 0) {
        int exit_code = exec_cshell_exit(cmd_tokens);
        log_command(cmd_tokens[0], exit_code);
    } else if (strcmp(cmd_tokens[0], builtin_commands[1]) == 0) {
        int exit_code = exec_cshell_log(cmd_tokens);
        log_command(cmd_tokens[0], exit_code);
    } else if (strcmp(cmd_tokens[0], builtin_commands[2]) == 0) {
        int exit_code = exec_cshell_print(cmd_tokens);
        log_command(cmd_tokens[0], exit_code);
    } else if (strcmp(cmd_tokens[0], builtin_commands[3]) == 0) {
        int exit_code = exec_cshell_theme(cmd_tokens);
        log_command(cmd_tokens[0], exit_code);
    }
}

void exec_non_builtin_command(char **cmd_tokens) {

    // Create a new child process;
    pid_t pid = fork();

    // Check if child process was succesfully created
    // If yes, execute the child process and
    // halt the parent process until child process is done
    if (pid == -1) {
        fprintf(stderr, "Error: fork() failed to create a process\n");
    } else if (pid == 0){
        execvp(cmd_tokens[0], cmd_tokens);
        // If execution of cmd failed, print out an error message and exit
        fprintf(stderr, "Error: Command failed to execute\n");
        exit(1);
    } else if (pid > 0) {
        int status;
        wait(&status);
        if (WIFEXITED(status)) {
            int exit_code = WEXITSTATUS(status);
            log_command(cmd_tokens[0], exit_code);
        }
    }
}


void exec_command(char *input, char **cmd_tokens) {

    for(int i = 0; i < builtin_commands_count; i++) {
        if(strcmp(cmd_tokens[0], builtin_commands[i]) == 0) {
            exec_builtin_command(cmd_tokens);
            return;
        }
    }

    if(cmd_tokens[0][0] == '$') {
        char input_str[BUFFER_SIZE];
        strcpy(input_str, input);
        char** env_var_tokens = parse_env_var(input);
        int exit_code = store_env_var(env_var_tokens);
        log_command(input_str, exit_code);

        free(env_var_tokens);
        return;
    }

    exec_non_builtin_command(cmd_tokens);
}

void exec_interactive_mode() {

    while(TRUE) {

        printf("cshell$ ");

        char *input = read_input();
        char input_buffer[BUFFER_SIZE];
        strcpy(input_buffer, input);
        char **cmd_tokens = tokenize(input);

        if(cmd_tokens[0]) {
            exec_command(input_buffer, cmd_tokens);
        }

        free(input);
        free(cmd_tokens);
    }
}

void exec_script_mode(char *file_name) {

    FILE *command_file = fopen(file_name, "r");
    if (command_file == NULL) {
        fprintf(stderr, "Error: fopen() failed to open testScript.txt\n");
    }

    char commands_buffer[BUFFER_SIZE];

    while (fgets(commands_buffer, BUFFER_SIZE, command_file) != NULL) {

        remove_new_line(commands_buffer);
        remove_carriage_return(commands_buffer);
        char temp_buffer[BUFFER_SIZE];
        strcpy(temp_buffer, commands_buffer);

        char **cmd_tokens = tokenize(commands_buffer);

        if(cmd_tokens[0]) {
            exec_command(temp_buffer, cmd_tokens);
        }

        free(cmd_tokens);
    }

    fclose(command_file);
}


int main(int argc, char **argv) {

    commands = (Command **)(malloc(sizeof(Command *) * commands_array_max_size));
    env_variables = (EnvVar **)(malloc(sizeof(EnvVar *) * env_variables_array_max_size));

    if(argc == 1) {
        exec_interactive_mode();
    } else {
        exec_script_mode(argv[1]);
    }


    for (int i = 0; i < commands_array_max_size; i++) {
        // free(commands[i]->name);
        free(commands[i]);
    }
    free(commands);

    for (int i = 0; i < env_variables_array_max_size; i++) {
        // free(env_variables[i]->name);
        // free(env_variables[i]->value);
        free(env_variables[i]);
    }
    free(env_variables);

    return 0;
}