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
    //printf("%d\n\n", index);
    

    int newSize = *len1 + len2-1; // size of the new char array

    //printf("%d\n",newSize);

    char** newCommand = (char**)malloc((newSize + 1) * sizeof(char*));

    if (newCommand == NULL){ // if malloc fails return NULL
        return NULL;
    }
    
    int temp = 0;

    for (int x = 0; x < newSize; x++){ // populates the new array
        if (x == index){
            for (int i = 0; i < len2; i++){ // inserts the elements from arguments when the index of the element with the wildcard is hit
                newCommand[x] = strdup(arguments[i]);
                //printf("%s - %d\n", newCommand[x], x);
                x++;
                temp++;
            }
            temp--;
            x--;
        }else{ // populates the new array with the elements from the original token array
            newCommand[x] = strdup(token[x-temp]);
            //printf("%s - %d\n", newCommand[x], x);
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
            //printf("%s\n", array[index]);
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

    /*if (strchr(after, "/")){ // If after contains a / the wild card is invalid
            //still need to add the error statement
    }*/

    
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

        //printf("- %s * %s\n", before, after);
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

    //printf("%s",slash);
    //printf("\n- %s\n\n", search_dir);

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
            //if we can't change to using glob then will need to update this above to add in the search_dir before adding the file that matches the pattern to the array
        }
    }

    //printf("%s -  %s \n\n", before, after);

    free(after);
    free(before);
    closedir(dir);

    
    /*for (int x = 0; x<numOfArgs; x++){
        printf("%s\n\n", argument_array[x]);
    }*/

    

    char **commandToken = str_to_array(command, numOfElements); // creates new string with tokens from command string


    /*for (int x = 0; x<*numOfElements; x++){
        printf("Token1 %u: %s\n",x+1, commandToken[x]);
    }*/

    //printf("%d - %d\n\n", *numOfElements, numOfArgs);

    char** newCommandToken = insertArguments(commandToken, numOfElements, argument_array, numOfArgs); // creates new string with all arguments found inserted in and replacing the old element with the wildcard

    /*int y = numOfArgs + *numOfElements -1;
    for (int x = 0; x<y; x++){
        printf("GG: %s - %d\n", newCommandToken[x], x);
    }*/


    if (strcmp(search_dir, ".")){ 
        free(search_dir);
    }
    freeArray(argument_array, numOfArgs);
    freeArray(commandToken, *numOfElements);
    
    *numOfElements = *numOfElements + numOfArgs - 1; // updates the numOfElements to the new len of the char array

    return newCommandToken;
}

//takes in command and redirects stdinput or stdoutput
int redirect_expansion(char **args, int numOfArgs, int *inRedirect, int *outRedirect){
    for (int i = 0; i < numOfArgs; i++) {
        if (strcmp(args[i], "<") == 0 && i + 1 < numOfArgs) {
            *inRedirect = open(args[i + 1], O_RDONLY);
            if (*inRedirect == -1) {
                perror("open (input redirection)");
                exit(EXIT_FAILURE);
            }
            args[i] = NULL;  
        } else if (strcmp(args[i], ">") == 0 && i + 1 < numOfArgs) {
            *outRedirect = open(args[i + 1], O_WRONLY | O_CREAT | O_TRUNC, 0640); // S_IRUSR|S_IWUSR|S_IRGRP
            if (*outRedirect == -1) {
                perror("open (output redirection)");
                exit(EXIT_FAILURE);
            }
            args[i] = NULL; 
        }
    }
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

int execute_command(char** args, int numOfArgs) {
    if(find_file(args) == -1) {
        fprintf(stderr, "Command not found: %s\n", args[0]);
        return -1;
    }

    int pipefd[2];
    int inRedirect = -1, outRedirect = -1;
    int pipePos = -1;

    // Check for pipe and redirections
    for(int i = 0; i < numOfArgs; i++) {
        if(strcmp(args[i], "|") == 0) {
            pipePos = i;
            args[i] = NULL;  // Split the command at the pipe
        }
    }

    redirect_expansion(args, numOfArgs, &inRedirect, &outRedirect);

    if(pipePos != -1) {
        // Create a pipe 
        if(pipe(pipefd) == -1) {
            perror("pipe");
            return -1;
        }
    }

    pid_t pid1 = fork();
    if(pid1 == -1) {
        perror("fork");
        return -1;
    }

    // rudimentary redirect, only handles one at a time
    /*for (int i = 0; i < numOfArgs; i++) {
        if (strcmp(args[i], "<") == 0 && i + 1 < numOfArgs) {
            inRedirect = open(args[i + 1], O_RDONLY);
            if (inRedirect == -1) {
                perror("open");
                return -1;
            }
        } else if (strcmp(args[i], ">") == 0 && i + 1 < numOfArgs) {
            outRedirect = open(args[i + 1], O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (outRedirect == -1) {
                perror("open");
                return -1;
            }
        }
    }*/
    
    if(pid1 == 0) {
        // First child process
        if (pipePos != -1) {
            dup2(pipefd[1], STDOUT_FILENO);
        }
        if (inRedirect != -1) {
            dup2(inRedirect, STDIN_FILENO);
            close(inRedirect);
        }
        if (outRedirect != -1 && pipePos == -1) {  // Apply outRedirect only if there's no pipe
            dup2(outRedirect, STDOUT_FILENO);
            close(outRedirect);
        }
        if (pipePos != -1) {
            close(pipefd[0]);
            close(pipefd[1]);
        }

        execv(args[0], args);
        perror("execv");
        exit(EXIT_FAILURE);
    } else {
        // Parent process
        if (pipePos != -1) {
            pid_t pid2 = fork();
            if (pid2 == -1) {
                perror("fork");
                return -1;
            }

            if (pid2 == 0) {
                // Second child process for the command after the pipe
                dup2(pipefd[0], STDIN_FILENO);
                close(pipefd[0]);
                close(pipefd[1]);

                char **secondCommand = &args[pipePos + 1];
                execv(secondCommand[0], secondCommand);
                perror("execv");
                exit(EXIT_FAILURE);
            } else {
                close(pipefd[0]);
                close(pipefd[1]);
            }
        }

        if (inRedirect != -1) {
            close(inRedirect);
        }
        if (outRedirect != -1) {
            close(outRedirect);
        }

        int status;
        waitpid(pid1, &status, 0); 
        if (pipePos != -1) {
            waitpid(pid2, &status, 0); 
        }
        return WEXITSTATUS(status);
    }
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
                }else{ // if buffer is a \n call .... and reset string to a \0
                    int e = 0;
                if (strcmp(line, "exit") == 0){ // checks for the exit command
                    write (STDOUT_FILENO, "mysh: exitting\n", 16);
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
                        write (STDOUT_FILENO, "mysh: exitting\n", 16);
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
                    write (STDOUT_FILENO, "mysh: exitting\n", 16);
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
            }else{ // if buffer is a \n call .... and reset string to a \0
                int e = 0;
                if (strcmp(line, "exit") == 0){ // checks for the exit command
                    write (STDOUT_FILENO, "mysh: exitting\n", 16);
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
                        write (STDOUT_FILENO, "mysh: exitting\n", 16);
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
                
            }
            
        }else if(r<0){ // Error case for read()
            perror("Error Reading from STDIN");
            break;
        }
    }
    free(line);


}






int main(int argc, char** argv){
    // Input loop that enters in to batch or interactive mode depending on the arguments
    /*char* command_input = "ls ten nine baz/BLUBBER.txt eight seven"; // if you remove baz/ it looks in the current directory and gets a runtime error
    int numOfElements = 0;
    
    char** temp = wildcard_expansion(command_input, &numOfElements);
    //printf("\n%d\n", numOfElements);
    for (int x = 0; x<numOfElements; x++){
        printf("Token %u: %s\n",x+1, temp[x]);
    }

    freeArray(temp, numOfElements);*/

    
    if (argc == 1){ // Interactive Mode call
        //printf("inter\n");
        interactive_mode();
    }

    if (argc > 1){ // Batch Mode call
        batch_mode(argc, argv); // might be able to remove the first for loop in batch as the assignment may only run batch for one argument and not multiple
    }


    return EXIT_SUCCESS;
}