#include<ctype.h>
#include<unistd.h>
#include<stdio.h>
#include<stdlib.h>
#include<fcntl.h>
#include<string.h>
#include<stddef.h>
#include<sys/stat.h>
#include<dirent.h>


// frees array contents and the array
void freeArray(char** array, int numOfElements){ 
    for (int x = 0; x<numOfElements; x++){
        free(array[x]);
    }
    free(array);
}


// Creates a new array by replacing the element with the wildcard in the token array with the contents of arguments.
char** insertArguments(char** token, int* len1, char** arguments, int len2) { 
    int index = 0;

    for (int x = 0; x<*len1; x++){ // finds the index of the element with the wildcard
        if (strstr(token[x], "*")){
            index = x;
        }
    }
    

    int newSize = *len1 + len2-1; // size of the new char array

    char** newCommand = (char**)malloc((newSize + 1) * sizeof(char*));

    if (newCommand == NULL){ // if malloc fails return NULL
        return NULL;
    }

    
    int temp = 0;


    for (int x = 0; x < newSize; x++){ // populates the new array
        if (x == index){
            for (int i = 0; i < len2; i++){ // inserts the elements from arguments when the index of the element with the wildcard is hit
                newCommand[x] = strdup(arguments[i]);
                x++;
                temp++;
            }
            temp--;
            x--;
        }else{ // populates the new array with the elements from the original token array
            newCommand[x] = strdup(token[x-temp]);
        }
    }

    return newCommand;
}


// converts a string to a char array
char** str_to_array(char *initial, int *numOfElements){
    char* space = " ";
    int count = 0;

    size_t len = strlen(initial);


    for (size_t x = 0; x<len; x++){ // finds the size that the array will need to be
        size_t tokenlen = strcspn(initial+x, space); // length until next space

        if (tokenlen > 0){
            count++;
            x += tokenlen;
        }
        x += strspn(initial+x, space)-1; // skips to the next space
    }


    *numOfElements = count; // updates the numOfElements
    char** array = (char**)malloc(count*sizeof(char*));


    if (array == NULL){ // if malloc fails return NULL
        return NULL;
    }


    int index = 0;


    for (int x = 0; x < len; x++){ // populates the char array with the words from the initial (char)
        size_t tokenlen = strcspn(initial+x, space);

        if (tokenlen > 0){
            array[index] = (char*)malloc((tokenlen + 1)); // allocates the necessary amount of memory for the string at the necessary index
            
            if (array[index] == NULL) { // if malloc ever fails the array and its contents will be freed
                for (int i = 0; i < index; i++) {
                    free(array[i]);
                }
                free(array);
                return NULL;
            }


            memcpy(array[index], initial+x, tokenlen); // populates the array index with the substring
            array[index][tokenlen] = '\0';
            tokenlen++;
            x += tokenlen-1;
            index++;
        }
        
        x += strspn(initial + x, space)-1; // skips to the next space
    }

    return array;
}

// takes in the command and expands the wildcard
char** wildcard_expansion(char* command, int* numOfElements)
{
    char* before;
    char* after;
    char* wildcard = strchr(command, '*');


    
    if (wildcard != NULL){ // finds the characters after the * but before the next space
        char* spaceAfter = strchr(wildcard, ' ');
        if (spaceAfter != NULL){ // if there is a space after the * populate after with the substring
            size_t lenA = spaceAfter - (wildcard +1);
            after = (char*)malloc(lenA+1);

            if (after != NULL){ // populates after with the substring after the *
                for (size_t i =0; i<lenA; i++){
                    after[i] = wildcard[i+1];
                }

                after[lenA] = '\0';
            }else{
                return NULL;
            }
        } else{ // if no substring after the * set after to be empty
            after = strdup(wildcard+1);
            if (after == NULL){
                after = "";
            }
        }
    }else if (wildcard == NULL){ // returns tokenized verion of the command input
        char** array = str_to_array(command, numOfElements);
        return array;
    }

    
    if (wildcard != NULL){ // finds the characters before the *
        char* spaceBefore = wildcard;
        
        while(*spaceBefore != ' '){ //finds the closest space before the *
            if(&spaceBefore == &command){
                break;
            } 
            spaceBefore = spaceBefore -1;
        }


        size_t lenB = wildcard - spaceBefore -1;
        before = (char*)malloc(lenB+1);

        if (before != NULL){ // finds the characters before the * but after the closest space
            for (size_t i =0; i<lenB; i++){
                before[i] = spaceBefore[i+1];
            }
            before[lenB] = '\0';
        }
    }


    char* slash = "";
    char* tempstr = before;
    while (*tempstr != '\0'){ // locates the last slash
        if (*tempstr == '/'){
            slash = tempstr;
        }
        tempstr++;
    }

    
    char* slicedBefore;
    if (strlen(slash) == 0){ // if no slash found there is no need to manipulate the string before 
        slicedBefore = before;
    }else{
        slicedBefore= slash+1; //before without the directory path
    }


    char* search_dir = ".";

    if (strcmp(slash, "") && strlen(slash)!= 0){ // gets the word with the * without the slash if there is one
        size_t lenS = slash - before +1;
        search_dir = (char*)malloc(lenS+2);
        strncpy(search_dir, before, lenS);
        search_dir[lenS] = '\0';
    }


    DIR *dir;
    struct dirent *entry;


    dir = opendir(search_dir);

    if (dir == NULL){ // error if dir fails to open
        write(STDOUT_FILENO, "ERROR failed to open directory", 31);
        return NULL;
    }

    
    char** argument_array = (char**)malloc(sizeof(char**));

    int numOfArgs = 0;

    
    while ((entry = readdir(dir)) != NULL){ // finds all files that follow the pattern
        if (entry->d_name[0] != '.' && strlen(entry->d_name) >= strlen(slicedBefore) && strlen(entry->d_name) >= strlen(after) && strstr((char*)(entry->d_name), slicedBefore) != NULL && strstr((char*)(entry->d_name), after) != NULL){
            numOfArgs++;
            argument_array = (char**)realloc(argument_array, sizeof(char**)*(numOfArgs+1));
            argument_array[numOfArgs-1] = strdup(entry->d_name); // adds applicable files to the argument array
        }
    }


    free(after);
    free(before);
    closedir(dir);

    char **commandToken = str_to_array(command, numOfElements); // creates new string with tokens from command string


    char** newCommandToken = insertArguments(commandToken, numOfElements, argument_array, numOfArgs); // creates new string with all arguments found inserted in and replacing the old element with the wildcard


    if (strcmp(search_dir, ".")){ 
        free(search_dir);
    }
    freeArray(argument_array, numOfArgs);
    freeArray(commandToken, *numOfElements);
    
    *numOfElements = *numOfElements + numOfArgs - 1; // updates the numOfElements to the new len of the char array

    return newCommandToken;
}

//takes in command and redirects stdinput or stdoutput
int redirect_expansion(char* command){





    
    return EXIT_SUCCESS;
}

int find_file(char **command) {
    // token contained /
    if (strchr(command[0], '/') != NULL) {
        return 0;
    }


    char *search_paths[] = {"/usr/local/bin", "/usr/bin", "/bin"};

    int bufferSize = 1;  // Initial buffer size
    char* path = malloc(bufferSize);
    path[0] = '\0';

    for (int i = 0; i < 3; i++) {
        bufferSize = strlen(search_paths[i]) + strlen(command[0]) + 2;  
        char *newPath = realloc(path, bufferSize);
        if (!newPath) {
            free(path);
            return -1; // Handle realloc failure
        }
        path = newPath;
        
        snprintf(path, bufferSize, "%s/%s", search_paths[i], command[0]); // Use bufferSize
        if (access(path, X_OK) == 0) {
            free(command[0]);
            command[0] = strdup(path);
            free(path);
            return 0;
        }
    }

    free(path);
    return -1; // Executable not found
}

int pwd(char** args, int numOfElements){ // built in pwd prints current working directory

    if (strcmp(args[0], "pwd") == 0){
           
        char cwd[5000];
        if (getcwd(cwd,sizeof(cwd)) != NULL){
            printf("%s\n", cwd);
        }else{
            perror("pwd");
        }
        
    }
    return EXIT_SUCCESS;
}

int which(char** args, int numOfElements){ // built in which prints the file path to listed command
    int built_in = 0;
    if (strcmp(args[0], "which") == 0){
        if (numOfElements == 2){
            if (strcmp(args[1], "which") == 0){
                printf("which: shell built-in command\n");
                built_in = 1;
            }else if (strcmp(args[1], "pwd") == 0){
                printf("pwd: shell built-in command\n");
                built_in = 1;
            }else if (strcmp(args[1], "cd") == 0){
                printf("cd: shell built-in command\n");
                built_in = 1;
            }else if (strcmp(args[1], "exit") == 0){
                printf("exit: shell built-in command\n");
                built_in = 1;
            }

            if (built_in == 0){
                if(((find_file(args+1) == 0) && (args[1] != NULL)) || numOfElements < 2){
                    printf("%s\n", args[1]);
                }else{
                    perror("which");
                }
            }
        }else{
            write(STDOUT_FILENO, "which: Incorrect number of arguments\n", 38);
        }
    }
    


    return EXIT_SUCCESS;
}

int cd(char** args, int numOfElements){ // built in cd changes directory

    if (strcmp(args[0], "cd") == 0){
        if (numOfElements == 2){
            if (args[1] == NULL || numOfElements > 2){
                write(STDERR_FILENO, "cd: too many arguments\n", 24);
            }else if (chdir(args[1]) != 0){
                perror("cd");
            }
        }else{
            write(STDERR_FILENO, "cd: Incorrect number of arguments\n", 35);
        }
    }

    return EXIT_SUCCESS;
}


int execute_command(char** command, int* numOfElements){

    if (strcmp(command[0], "cd") == 0){
        cd(command, *numOfElements);
    }else if (strcmp(command[0], "pwd") == 0){
        pwd(command, *numOfElements);
    }else if (strcmp(command[0], "which") == 0){
        which(command, *numOfElements);
    }

    freeArray(command, *numOfElements);
    return EXIT_SUCCESS;
}

//uses pipe() to transfer stdoutput to standard input of the next job
int execute_pipe_command(char** command, int* numOfElements){

    for (int x = 0; x<*numOfElements; x++){
        printf("TokenPIPE %u: %s\n",x+1, command[x]);
    }

    freeArray(command, *numOfElements);
    return EXIT_SUCCESS;
}

int batch_mode(int numOfArgs, char** arguments){

    int fd;
    struct stat fileStat;
    char buffer;
    int line_len = 1;
    char *line = (char*)malloc(1); // initialize as an empty string
    line[0] = '\0';

    for (int x = 1; x<numOfArgs; x++){ // might be able to remove the loop as the assignment may only run batch for one argument and not multiple
        char *path = arguments[x];
        int file_state = -1;

        if (stat(path, &fileStat) == 0){
            if (S_ISDIR(fileStat.st_mode)){ // checks if argument is a directory
                file_state = -1;
            }
            else if (S_ISREG(fileStat.st_mode)){ // checks if argument is a regular file
                file_state = 1;
            }
        }
        if (file_state == -1){ // if it is not a regular file skip
            write(STDOUT_FILENO, "ERROR incorect file type\n",26);
            write(STDOUT_FILENO, "Arguments should be a regular File\n",36);
            continue;
        }

        fd = open(path, O_RDONLY);

        if (fd == -1){ // ERORR if fails to open file
            write(STDOUT_FILENO, "ERROR opening files!\n",21);
            return EXIT_FAILURE;
        }

        if (file_state){
            int e = 0;
            while (read(fd, &buffer, 1) != 0){ // reads through the file one byte at a time 
                if (buffer != '\n'){ // if buffer is not a \n add the char to the line string
                    line = (char*)realloc(line, line_len+1);
                    line[line_len-1] = buffer;
                    line[line_len] = '\0';
                    line_len++;
                }else if (line_len > 1){ // if buffer is a \n call .... and reset string to a \0
                    int e = 0;
                    if (strcmp(line, "exit") == 0){ // checks for the exit command
                        write (STDOUT_FILENO, "mysh: exiting\n", 15);
                        free(line);
                        exit(EXIT_SUCCESS);
                    }else if (strstr(line, "exit")){ // search to see if exit is the first word on the command
                        char* tmp_line = line;
                        e = 0;
                        for (int i = 0; i < strlen(line); i++){
                            if (line[i] == ' '){
                                tmp_line++;
                            }else if (strstr(tmp_line, "exit") && (tmp_line[4] == ' ' || tmp_line[4] == '\0')){ // checks to see if exit is the only word in the line
                                e = 1;
                                i+=3;
                            }else if(line != strstr(line, "exit") && line[i] != ' '){ // exit is not at the start
                                e = 0;
                                break;
                            }else{
                                e = -1; // there are to many arguments
                                break;
                            }
                        }
                        if (e == 1){
                            write (STDOUT_FILENO, "mysh: exiting\n", 15);
                            free(line);
                            exit(EXIT_SUCCESS);
                        }else if (e == -1){
                            write (STDOUT_FILENO, "exit: too many arguments\n", 26);
                        }
                    }
                    if (e == 0){
                        char** line_token = wildcard_expansion(line, &line_len);

                        if(strstr(line, "|")){
                            execute_pipe_command(line_token, &line_len);
                        }
                        else{
                            execute_command(line_token, &line_len);
                        }
                    }
                    line = (char*)realloc(line, 1);
                    line[0] = '\0';
                    line_len = 1;
                }
                
            }
            if (strlen(line)>1){ // if the last line is not a \0  call ... and reset string to a \0
                if (strcmp(line, "exit") == 0){
                    write (STDOUT_FILENO, "mysh: exiting\n", 15);
                    break;
                }

                if (e == 0){
                    char** line_token = wildcard_expansion(line, &line_len);
                    if(strstr(line, "|")){
                        execute_pipe_command(line_token, &line_len);
                    }
                    else{
                        execute_command(line_token, &line_len);
                    }
                }
                line = (char*)realloc(line, 1);
                line[0] = '\0';
                line_len = 1;
            }
        }
        free(line);
        close(fd);
    }

    return EXIT_SUCCESS;
}


void interactive_mode(){
    char buffer;
    int r;
    char *line = (char*)malloc(1); // initialize as an empty string
    line[0] = '\0'; // ensure null terminated
    int line_len = 1;

    write(STDOUT_FILENO, "Welcome to mysh!\n",18);
    write(STDOUT_FILENO, "mysh> ", 7);
    while(1){
        if ((r = read(STDIN_FILENO, &buffer, 1)) != 0){ // reads through the file one byte at a time 
            if (buffer != '\n'){ // if buffer is not a \n add the char to the line string
                line = (char*)realloc(line, line_len+1);
                line[line_len-1] = buffer;
                line[line_len] = '\0';
                line_len++;
            }else if (line_len > 1){ // if buffer is a \n call .... and reset string to a \0
                int e = 0;
                if (strcmp(line, "exit") == 0){ // checks for the exit command
                    write (STDOUT_FILENO, "mysh: exiting\n", 15);
                    exit(EXIT_SUCCESS);
                }else if (strstr(line, "exit")){ // search to see if exit is the first word on the command
                    char* tmp_line = line;
                    
                    for (int i = 0; i < strlen(line); i++){
                        if (line[i] == ' '){
                            tmp_line++;
                        }else if (strstr(tmp_line, "exit") && (tmp_line[4] == ' ' || tmp_line[4] == '\0')){
                            e = 1;
                            i+=3;
                        }else if(line != strstr(line, "exit") && line[i] != ' '){ // exit is not at the start
                            e = 0;
                            break;
                        }else{
                            e = -1;
                            break;
                        }
                    }
                    if (e == 1){
                        write (STDOUT_FILENO, "mysh: exiting\n", 15);
                        exit(EXIT_SUCCESS);
                    }else if (e == -1){
                        write (STDOUT_FILENO, "exit: too many arguments\n", 26);
                    }
                }

                
                if (e == 0){
                    line_len--;
                    char** line_token = wildcard_expansion(line, &line_len); // converts line to a char array and if there is a * present expands the wildcard
                    
                    if(strstr(line, "|")){ 
                        execute_pipe_command(line_token, &line_len);
                    }
                    else{
                        execute_command(line_token, &line_len);
                    }
                }    
                line = (char*)realloc(line, 1);
                line[0] = '\0';
                line_len = 1;
                write(STDOUT_FILENO, "mysh> ", 7);
                
            }else{
                write(STDOUT_FILENO, "mysh> ", 7);
            }
            
        }else if(r<0){ // Error case for read()
            perror("Error Reading from STDIN");
            break;
        }
    }
    free(line);


}

int main(int argc, char** argv){
    
    if (argc == 1){ // Interactive Mode call
        interactive_mode();
    }

    if (argc > 1){ // Batch Mode call
        batch_mode(argc, argv); // might be able to remove the first for loop in batch as the assignment may only run batch for one argument and not multiple
    }


    return EXIT_SUCCESS;
}