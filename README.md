# MYSH
## CS 214 Fall 2023

## Contributors:
### Matthew Bixby - mjb593
### Serge Kuznetsov - sak339

## How to run tests:

#### run 'make' to create mysh
#### run './mysh' to run in interactive mode
#### run './mysh `{file_name}`' to run in batch mode

## Design Choices:

One of the design choices we made was to manually code the wildcard expansion instead of using glob,
as we wanted a full understanding of how the code worked.
We also integrated piping directly into the execute_command function, as only one pipe needed to be 
dealth with at a time.

## Test Strategy and Test Cases:

Our testing strategy was to make sure all commands worked for both batch and interactive mode. We 
created intensive testing files that testing the correctness of piping and redirection and how they
interacted with each other. We also tested the thoroughness of the wildcard expansion by making 
layers of directories and test files and making sure mysh found and ran every command. We tested these  
commands manually in interact mode as well.

