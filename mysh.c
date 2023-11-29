#include<ctype.h>
#include<unistd.h>
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<stddef.h>
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
        return str_to_array(command, numOfElements);
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
int redirect_expansion(char* command){






    return EXIT_SUCCESS;
}

//uses pipe() to transfer stdoutput to standard input of the next job
int pipe_expansion(char* command){








    return EXIT_SUCCESS;
}









int main(int argc, char** argv){
    // Input loop that enters in to batch or interactive mode depending on the arguments
    char* command_input = "ls ten nine baz/BLUBBER*.txt eight seven"; // if you remove baz/ it looks in the current directory and gets a runtime error
    int numOfElements = 0;
    
    char** temp = wildcard_expansion(command_input, &numOfElements);
    //printf("\n%d\n", numOfElements);
    for (int x = 0; x<numOfElements; x++){
        printf("Token %u: %s\n",x+1, temp[x]);
    }

    freeArray(temp, numOfElements);
    return EXIT_SUCCESS;
}