#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <sys/stat.h>
#include <dirent.h>
#include <limits.h>
#include <sys/types.h>
#include <fcntl.h>

#define CREATE_FLAGS (O_WRONLY | O_CREAT | O_APPEND)

#define CREATE_MODE (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)

#define MAX_LINE 80

void setup(char inputBuffer[], char *args[], int *background)
{

    int length, /* # of characters in the command line */
        i,      /* loop index for accessing inputBuffer array */
        start,  /* index where beginning of next command parameter is */
        ct;     /* index of where to place the next parameter into args[] */

    ct = 0;

    /* read what the user enters on the command line */
    length = read(STDIN_FILENO, inputBuffer, MAX_LINE);

    start = -1;
    if (length == 0)
        exit(0); /* ^d was entered, end of user command stream */

    if ((length < 0) && (errno != EINTR))
    {
        perror("error reading the command");
        exit(-1); /* terminate with error code of -1 */
    }

    /*printf(">>%s<<",inputBuffer);*/
    for (i = 0; i < length; i++)
    { /* examine every character in the inputBuffer */

        switch (inputBuffer[i])
        {
        case ' ':
        case '\t': /* argument separators */
            if (start != -1)
            {
                args[ct] = &inputBuffer[start]; /* set up pointer */
                ct++;
            }
            inputBuffer[i] = '\0'; /* add a null char; make a C string */
            start = -1;
            break;

        case '\n': /* should be the final char examined */
            if (start != -1)
            {
                args[ct] = &inputBuffer[start];
                ct++;
            }
            inputBuffer[i] = '\0';
            args[ct] = NULL; /* no more arguments to this command */
            break;

        default: /* some other character */
            if (start == -1)
                start = i;
            if (inputBuffer[i] == '&')
            {
                *background = 1;
                inputBuffer[i - 1] = '\0';
            }
        }            /* end of switch */
    }                /* end of for */
    args[ct] = NULL; /* just in case the input line was > 80 */

    //for (i = 0; i <= ct; i++)
    //printf("args %d = %s\n",i,args[i]);
}

void getpath(char *args[], char *envs[], char *path)
{

    char pathiki[130];
    path[0] = '\0';
    pathiki[0] = '\0';

    char cwd[PATH_MAX];
    if (getcwd(cwd, sizeof(cwd)) != NULL)

        for (int j = 0; j < 10; j++)
        {
            chdir(envs[j]);

            if (access(args[0], F_OK) != -1)
            {
                strcpy(path, envs[j]);
                strcat(path, "/");
                strcat(pathiki, args[0]);
                strcat(path, pathiki);
            }
        }

    chdir(cwd);

    return;
}

void execfunc(int background, char path[], char *args[])
{

    if (strcmp(args[0], "exit") == 0)
    {
        exit(0);
    }
    if (strcmp(args[0], "clr") == 0)
    {
        system("clear");
    }

    pid_t childpid;
    childpid = fork();

    if (childpid == 0)
    {

        for (int ck = 0; args[ck] != NULL; ck++)
        {

            if (strcmp(args[ck], "<") == 0)
            {
                int fd;
                fd = open(args[ck + 1], O_RDONLY);
                dup2(fd, STDIN_FILENO);
                close(fd);
                args[ck] = NULL;
                continue;
            }
            if (strcmp(args[ck], ">>") == 0)
            {

                int fd;
                fd = open(args[ck + 1], (O_WRONLY | O_CREAT | O_APPEND), CREATE_MODE);
                dup2(fd, STDOUT_FILENO);

                close(fd);
                args[ck] = NULL;
                continue;
            }
            if (strcmp(args[ck], "2>") == 0)
            {

                int fd;
                fd = open(args[ck + 1], (O_WRONLY | O_CREAT | O_TRUNC), CREATE_MODE);
                dup2(fd, STDERR_FILENO);

                close(fd);
                args[ck] = NULL;
                continue;
            }
            if (strcmp(args[ck], ">") == 0)
            {

                int fd;
                fd = open(args[ck + 1], (O_WRONLY | O_CREAT | O_TRUNC), CREATE_MODE);
                dup2(fd, STDOUT_FILENO);

                close(fd);
                args[ck] = NULL;
                continue;
            }
        }
        if (strcmp(path, "") == 0)
        {
            printf("Please enter valid command\n");
            exit(0);
        }

        execl(path, args[0], args[1], args[2],
              args[3], args[4], args[5], args[6],
              args[7], args[8], args[9], args[10],
              args[11], args[12], args[13], args[14],
              args[15], args[16], args[17], args[18],
              args[19], args[20], args[21], args[22],
              args[23], args[24], args[25], args[26],
              args[27], args[28], args[29], args[30],
              args[31], args[32], args[33], args[34],
              args[35], args[36], args[37], args[38],
              args[39], args[40], args[41]);
    }
    else
    {
        //TODO eger background 1 se childpidleri linkedliste at.
        if (background == 0)
            waitpid(childpid, NULL, 0);
        else
            waitpid(childpid, NULL, WNOHANG);
    }
}

int main(void)
{
    int ikk = 0;
    char path1[50];
    char argsfun1[50];
    char inputBuffer[MAX_LINE]; /*buffer to hold command entered */
    int background;             /* equals 1 if a command is followed by '&' */
    char *args[MAX_LINE / 2 + 1];

    const char *s = getenv("PATH");
    int i = 0;
    char *p = strtok(s, ":");
    char *array[10];

    while (p != NULL)
    {
        array[i++] = p;

        p = strtok(NULL, ":");
    }

    while (1)
    {

        fflush(stdout);
        background = 0;
        printf("myshell: ");
        fflush(stdout);
        setup(inputBuffer, args, &background);

        if (args[0] == NULL)
        {
            printf("Please enter a value.\n");
            continue;
        }
        getpath(args, array, path1);
        execfunc(background, path1, args);
    }
}
