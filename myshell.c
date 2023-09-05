/*
	Name - Jonathan Mathurin
	Panther ID - 6169303

	I affirm that I wrote this program myself without any help from any other
	people or sources from the internet.

	------------------------------------------------

    This program is intended to demonstrate how to use I/O redirections in C.
*/

#include <stdio.h>
#include <sys/wait.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>

#define MAX_ARGS 20
#define BUFSIZE 1024


/* A simple function utilized to populate an argument. */
void pop_arg(char** arg1, char** arg2, int index) {
    printf("arg1 = %s", arg1[0]);
    for(int o = 0; o < index; o++) {
        arg2[o] = arg1[o];
        if (o == (index - 1)) {
            arg2[o + 1] = 0;
        }
    }
}


int get_args(char* cmdline, char* args[])
{
    int i = 0;

    /* if no args */
    if((args[0] = strtok(cmdline, "\n\t ")) == NULL)
        return 0;

    while((args[++i] = strtok(NULL, "\n\t ")) != NULL) {
        if(i >= MAX_ARGS) {
            printf("Too many arguments!\n");
            exit(1);
        }
    }
    /* the last one is always NULL */
    return i;
}

void execute(char* cmdline) {
    /* FileId is a variable I added to ensure that > overwriting  worked as intended.*/
    int pid, async, fileId;

    /* Input - A variables to store the fd for input file
     * Output - Stores the fd for output file
     * sub_args - A new list meant to store commands as I trim args down. */
    int Input, Output, numOfPipes = 0;
    char *args[MAX_ARGS];
    char *sub_args[MAX_ARGS];

    int nargs = get_args(cmdline, args);
    if (nargs <= 0) return;

    if (!strcmp(args[0], "quit") || !strcmp(args[0], "exit")) {
        exit(0);
    }
    /* check if async call */
    if (!strcmp(args[nargs - 1], "&")) {
        async = 1;
        args[--nargs] = 0;
    } else async = 0;

    /* A simple for loop used to identify if our commands will have pipes.
     * If it does have pipes, we keep track of the total number for future use. */
    for (int i = 0; i < nargs; i++) {
        if (!strcmp(args[i], "|")) {
            numOfPipes++;
        }
    }


    /* If we have any pipes, we're going to run an operation with pipes in mind. */
    if (numOfPipes > 0) {
        int pid, status;
        int fd[2];

        /* This is just a mechanism we use to get the input and outputs*/
        for (int i = 0; i < nargs; i++) {
            /* If we find the output operator, we know that the next item in the list
             * will be name of the output file to be created/overwritten. */
            if (!strcmp(args[i], ">")) {
                //This stores the name into output.
                //Creat ensures that the file is overwritten.
                Output = creat(args[i + 1], 0640);
            }
            /* If we find the input operator, we know that the next item in the list
            * will be name of the input file to be used as an input. */
            if (!strcmp(args[i], "<")) {
                //Stores name into input and makes it read only.
                Input = open(args[i + 1], O_RDONLY, 0666);
            }
        }


        pipe(fd);
        switch (pid = fork()) {
            case 0: /* child */
                dup2(fd[1], 1);    /* this end of the pipe becomes the standard output */
                close(fd[0]);        /* this process does not need the other end */


                /* Run a loop from the beginning of the array, until the end. */
                for (int i = 0; i < nargs; i++) {

                    //Once e find the pipe, we can begin the operations.
                    if (!strcmp(args[i], "|")){

                        /* We add to a sub_arg list from the beginning up until the pipe.*/
                        for (int o = 0; o < i; o++) {
                            sub_args[o] = args[o];
                            if (o == (i - 1)) {

                                //Add null character to end.
                                sub_args[o + 1] = 0;
                            }
                        }
                    }
                }
                execvp(sub_args[0], sub_args);    /* run the command */
                fprintf(stderr, "%s failed\n", sub_args[0]);    /* commandt failed! */

            default: /* parent does nothing */
                break;

            case -1:
                fprintf(stderr, "fork failed\n");
                exit(1);
        }
        switch (pid = fork()) {
            case 0: /* child */
                dup2(fd[0], 0);    /* this end of the pipe becomes the standard input */
                close(fd[1]);		/* this process doesn't need the other end */

                for (int i = 0; i < nargs; i++) {
                    if (!strcmp(args[i], "|")){
                        /* Once we locate the pipe, we focus on the opposite end as we did before.
                         * Hence the loop looks beyond the pipe and goes to the end of the argument.*/
                        int index_1 = 0;
                        for (int n = i+1; n < nargs; n++) {
                            sub_args[index_1] = args[n];
                            if (n == (nargs - 1)) {
                                sub_args[index_1 + 1] = 0;
                            }
                            index_1++;
                        }
                    }
                }
                execvp(sub_args[0], sub_args);    /* run the command */
                fprintf(stderr, "%s failed\n", sub_args[0]);    /* it failed! */

            default: /* parent does nothing */
                break;

            case -1:
                fprintf(stderr, "fork failed\n");
                exit(1);
        }

        close(fd[0]);
        close(fd[1]);    /* this is important! close both file descriptors on the pipe */
        while ((pid = wait(&status)) != -1);
        //fprintf(stderr, "process %d exits with %d\n", pid, status);

    }

    /* If there are no pipes, we do not need to run as many forks. Instead we will simply
     * trim the args list, and operate the commands one by one. */
    else {

        pid = fork();
        if (pid == 0) { /* child process */
            for (int i = 0; i < nargs; i++) {

                /* If we find the output operator, we know that the next item in the list
                 * will be name of the output file to be created/overwritten. */
                if (!strcmp(args[i], ">")) {
                    //This stores the name into output.
                    //Creat ensures that the file is overwritten.
                    Output = creat(args[i + 1], 0640);
                }

                /* If we find the input operator, we know that the next item in the list
                * will be name of the input file to be used as an input. */
                if (!strcmp(args[i], "<")) {

                    //Stores name into input and makes it read only.
                    Input = open(args[i + 1], O_RDONLY, 0666);
                }

            }

            /* Next we run a for loop to populate sub_args with a sublist of each command.
             * In this particular loop, we're simply looking to keep track of the unix commands called
             * before the output redirection.
             *
             * These commands are then stored, and the null character is added at the end. Finally,
             * we run the operation using execvp. */
            for (int i = 0; i < nargs; i++) {
                if (!strcmp(args[i], ">")) {
                    //The simple loop to populate sub_arg.
                    pop_arg(args, sub_args, i);
                    /* Here we simply change the stream from stdin, to out input file.
                     * Then we also change our stream from stdout to our output file.
                     * */
                    if(Input){
                        dup2(Input, fileno(stdin));
                        close(Input);
                    }
                    if(Output){
                        dup2(Output, fileno(stdout));
                        close(Output);
                    }
                    execvp(sub_args[0], sub_args);
                }

                /* The operations here function virtually in the same way.
                 * However, instead, we call open so that we can append to a file, rather than overwrite. */
                if (!strcmp(args[i], ">>")) {
                    pop_arg(args, sub_args, i);

                    //fileID is equal to the name of file, but can be appended to.
                    fileId = open(args[i + 1], O_RDWR | O_APPEND, 0666);

                    //Finally we change the stream
                    dup2(fileId, fileno(stdout));
                    close(fileId);

                    //Command outputs to the file.
                    execvp(sub_args[0], sub_args);
                }

                /* Same sequence as above. This time we change the stdin to our file.*/
                if (!strcmp(args[i], "<")) {
                    pop_arg(args, sub_args, i);

                    if(Input){
                        dup2(Input, fileno(stdin));
                        close(Input);
                    }
                    if(Output){
                        dup2(Output, fileno(stdout));
                        close(Output);
                    }
                    execvp(sub_args[0], sub_args);
                    //Run command.
                    execvp(sub_args[0], sub_args);
                }
            }
            //If all the above isn't necessary, the command can still run as intended.
            execvp(args[0], args);
            perror("exec failed");
            exit(-1);
        }
        else if (pid > 0) { /* parent process */
            if (!async) waitpid(pid, NULL, 0);
            else printf("this is an async call\n");
        }

        else { /* error occurred */
            perror("fork failed");
            exit(1);

        }

    }
}
int main (int argc, char* argv [])
{
    char cmdline[BUFSIZE];

    for(;;) {
        printf("COP4338$ ");
        if(fgets(cmdline, BUFSIZE, stdin) == NULL) {
            perror("fgets failed");
            exit(1);
        }
        execute(cmdline) ;
    }
    return 0;
}