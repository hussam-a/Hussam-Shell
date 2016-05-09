/*
Part of Hussam-Shell (github.com/hussam-a/Hussam-Shell)
Authors:
Hussam A. El-Araby (github.com/hussam-a)

Project code and configurations are under the GNU GPL license (circ-min/LICENSE) 
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>

#define MAX_ARGS 40     /*Maximum of 39 arguments are allowed per command (not counting the command itself)*/

int main(void)
{
    char *args[MAX_ARGS];   //Array to hold the command and its arguments tokenized
    int history_numbers[10];  //Array to hold index of entry of the commands
    char* history_lines[10];  //Array to hold history of commands and their arguments
    memset(history_lines, 0, sizeof(history_lines));

    char* line = NULL;  //Char pointer to hold a single command and its arguments
    char * line_unmodified = NULL;  //Char pointer to hold the same as char * line  but does not get modified
    int exec_count = 0;     //Counter of the number of commands executed
    int front_queue = -1;   //Countner to the start of the queue (the newest command executed)
    int back_queue =  -1;   //Countner to the start of the queue (the old command executed)

    while (1)
    {
        printf("hussamshell> ");    //Command prompt
        fflush(stdout);     //Flush any output to stdout
        if (line!= NULL) free(line);
        line = NULL;
        size_t number_to_read = 0;
        getline(&line, &number_to_read,stdin);  //Read command from user, delimiter is newline

parse_line:
        if (line_unmodified!= NULL) free(line_unmodified);
        line_unmodified = (char *) calloc (strlen(line)+1, sizeof(char));
        strcpy(line_unmodified, line);

        //Tokenize the char * line into its arguments using the args array
        int arg_count = 0;
        args[arg_count] = strtok (line," \n");
        while (args[arg_count] != NULL)
        {
            if (arg_count>=MAX_ARGS-1) break;
            arg_count++;
            args[arg_count] = strtok (NULL, " \n");
        }

        //Check if the user wants to exit, break out of while loop
        if (strcmp(args[0], "exit") == 0) {break;}

        //Check if user wants history of command to be output
        if (strcmp(args[0], "history") == 0)
        {
                //If no commands have been executed, output error and continue to prompt user for another command
                if (exec_count==0)
                {
                    printf("%s\n", "No commands in history.");
                    continue;
                }
                else
                {
                    //Iterate over history in the correct order to view it
                    int i = front_queue;
                    for (; i >= back_queue ; i-- )
                    {
                        printf("%d %s\n", history_numbers [i], history_lines[i]);
                    }
                    if (back_queue>front_queue)
                    {
                        i = front_queue;
                        for (; i >= 0 ; i-- )
                        {
                            printf("%d %s\n", history_numbers [i], history_lines[i]);
                        }
                        i = 9;
                        for (; i >= back_queue ; i-- )
                        {
                            printf("%d %s\n", history_numbers [i], history_lines[i]);
                        }
                    }
                    continue;   //Skip the rest of the loop, go ask user for another command
                }
        }

        //Check for first exclamation mark
        if (strcmp(args[0], "!") == 0)
        {
            //Check for second exclamation mark
            if (strcmp(args[1], "!") == 0)
            {
                //If ! ! and there is atleast one executed command
                if (exec_count>0)
                {
                    //Find the last command and execute it from history_lines array
                    if (line!= NULL) free(line);
                    line = (char *) calloc (strlen(history_lines[front_queue])+1, sizeof(char));
                    strcpy(line, history_lines[front_queue]);
                    goto parse_line;    //Jump to part after command prompt in command parsing
                }
                else
                {
                    //Otherwise if ! ! but no commands have been executed, output error and continue to ask for new command
                    printf("%s\n", "No commands in history.");
                    continue;
                }
            }
            else
            {
               //Otherwise if ! followed by another argument
               //Check if its a valid number for a command that have been recently executed
               if (((exec_count -  atoi(args[1])) < 10) && ((exec_count -  atoi(args[1]))>=0) && (exec_count>0))
                {
                   //If so, go execute it, same way as explained above for ! !
                   line = (char *) calloc (strlen(history_lines[(atoi(args[1])-1)%10])+1, sizeof(char));
                   strcpy(line, history_lines[(atoi(args[1])-1)%10]);
                   goto parse_line;
                }
                else
                {
                    //Else if , ! x is not a valid command in the history , output error and continue to ask for new command
                    printf("%s\n", "No such command in history.");
                    continue;
                }
            }
        }

        //Program reaches here only in case the command input was not ! x or ! ! or history

        //Set parent to wait if & is not the last argument in the command parsed
        int wait_or_not = 1;
        if (strcmp(args[arg_count-1], "&") == 0) {wait_or_not = 0; args[arg_count-1] = NULL;}

        //Fork
        pid_t pid = fork();

        //Check if parent or child
        if (pid == 0)
        {
            //If child try executing the program in args array
            //If child fails it outputs an error and exits with an error to indicate to the parent process the failure.
            if (execvp(args[0], args)==-1) printf("%s\n", "Child reports: failed to execute.");
            exit(1);
        }
        else
        {
            //If parent, check if fork did indeed occur or not
            if (pid == -1)
            {
                //Report if fork failed
                printf("%s\n", "Parent reports: failed to fork.");
            }
            else
            {
               //Successful fork
               exec_count++;    //Increase the number of (tried to) execute commands
               front_queue = front_queue + 1;   //Adjust the queue counters
               front_queue = front_queue%10;
               history_numbers[front_queue] = exec_count;   //Place the command and its number in the appropriate arrays.
               if (history_lines[front_queue] != NULL) free(history_lines[front_queue]);
               history_lines[front_queue] = (char *) calloc (strlen(line_unmodified)+1, sizeof(char));
               strcpy(history_lines[front_queue], line_unmodified);
               if (exec_count == 1) {back_queue = 0;}   //Adjust special case for back_queue counter
               if (exec_count > 10) {back_queue = back_queue + 1; back_queue = back_queue%10;}  //Adjust special case for back_queue counter
               if (wait_or_not) //Only enter this, when the parent is set to wait  (& sign was not present as last argument)
               {
                   int status;
                   //Wait for child
                   waitpid(pid, &status,0);
                   //Report if child failed to execute program in args , as the child is supposed to do so.
                   if (status) { printf("%s\n", "Parent reports: failed child.");}
               }
            }
        }
    }   //Loop if the exit was not input

    return 0;   //Return when user write exit command
}
