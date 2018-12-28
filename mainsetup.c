

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
#include <signal.h>

#define CREATE_FLAGS (O_WRONLY | O_CREAT | O_APPEND)
#define CREATE_MODE (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)
#define MAX_LINE 80
#define MAX 255

struct Node //Struct for keep alias variables.
{
    char inquote[MAX];  //for real variable  "ls -l"
    char outquote[MAX]; //for fake variable "list"
    struct Node *next;
};
typedef struct Node NodeAlias;
NodeAlias *aliasHead = NULL;

struct pidlist //Struct for keep childpids for waiting parent pids.
{
    pid_t pid;
    struct pidlist *next;
};
typedef struct pidlist NodePidList;
NodePidList *pidlistHead = NULL;

void pushAlias(NodeAlias **head_ref, char inquote[255], char outquote[255]) //Alias linked list push method
{
    /* 1. allocate node */
    NodeAlias *new_node = (NodeAlias *)malloc(sizeof(NodeAlias));

    NodeAlias *last = *head_ref; /* used in step 5*/

    /* 2. put in the data  */
    strcpy(new_node->inquote, inquote);
    strcpy(new_node->outquote, outquote);

    /* 3. This new node is going to be the last node, so make next of 
          it as NULL*/
    new_node->next = NULL;

    /* 4. If the Linked List is empty, then make the new node as head */
    if (*head_ref == NULL)
    {
        *head_ref = new_node;
        return;
    }

    /* 5. Else traverse till the last node */
    while (last->next != NULL)
        last = last->next;

    /* 6. Change the next of last node */
    last->next = new_node;
    return;
}

void pushpidlist(NodePidList **head_ref, pid_t a) //Pid list push method
{
    /* 1. allocate node */
    NodePidList *new_node = (NodeAlias *)malloc(sizeof(NodeAlias));

    NodePidList *last = *head_ref; /* used in step 5*/

    /* 2. put in the data  */
    new_node->pid = a;

    /* 3. This new node is going to be the last node, so make next of 
          it as NULL*/
    new_node->next = NULL;

    /* 4. If the Linked List is empty, then make the new node as head */
    if (*head_ref == NULL)
    {
        *head_ref = new_node;
        return;
    }

    /* 5. Else traverse till the last node */
    while (last->next != NULL)
        last = last->next;

    /* 6. Change the next of last node */
    last->next = new_node;
    return;
}

void deleteAlias(NodeAlias **head_ref, char key[255]) //Delete method for alias list
{
    // Store head node
    NodeAlias *temp = *head_ref, *prev;

    // If head node itself holds the key to be deleted
    if (temp != NULL && !strcmp(temp->outquote, key))
    {
        *head_ref = temp->next; // Changed head
        free(temp);             // free old head
        return;
    }

    while (temp != NULL && strcmp(temp->outquote, key))
    {
        prev = temp;
        temp = temp->next;
    }

    // If key was not present in linked list
    if (temp == NULL)
        return;

    // Unlink the node from linked list
    prev->next = temp->next;

    free(temp); // Free memory
}

void deletepidlist(NodePidList **head_ref, pid_t a) //Delete method for pid list
{
    // Store head node
    NodePidList *temp = *head_ref, *prev;

    // If head node itself holds the key to be deleted
    if (temp != NULL && temp->pid != a)
    {
        *head_ref = temp->next; // Changed head
        free(temp);             // free old head
        return;
    }

    while (temp != NULL && temp->pid == a)
    {
        prev = temp;
        temp = temp->next;
    }

    // If key was not present in linked list
    if (temp == NULL)
        return;

    // Unlink the node from linked list
    prev->next = temp->next;

    free(temp); // Free memory
}

void printList(NodeAlias **node) //For alias -l command
{
    NodeAlias *tempNode = *node;
    while (tempNode != NULL)
    {
        printf("%s  \"%s\"\n", tempNode->outquote, tempNode->inquote);
        tempNode = tempNode->next;
    }
}

void setup(char inputBuffer[], char *args[], int *background) //Given setup function
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

    // for (i = 0; i <= ct; i++)
    //   printf("args %d = %s\n", i, args[i]);
}

void getpath(char *args[], char *envs[], char *path) //Return path from given command (with args array)
{                                                    //function take all paths with path array.

    char pathiki[130]; //for strcat method. Strcat didnt run  when using one variable
    path[0] = '\0';    //get empty array
    pathiki[0] = '\0';

    char cwd[PATH_MAX];                   //for keeping current directory
    if (getcwd(cwd, sizeof(cwd)) != NULL) //keep currently running directory in cwd variable

        for (int j = 0; j < 10; j++)
        {                   //searching all paths
            chdir(envs[j]); //go each path and

            if (access(args[0], F_OK) != -1) //control path if its true path
            {
                strcpy(path, envs[j]);    // /usr/bin
                strcat(path, "/");        // /
                strcat(pathiki, args[0]); // ls
                strcat(path, pathiki);    // /usr/bin/ls
            }
        }

    chdir(cwd); //return first directory(currently running for example Desktop)

    return; //path=commands path
}

void execfunc(int background, char path[], char *args[])
{

    if (strcmp(args[0], "exit") == 0)
    {
        exit(0);
    }
    if (strcmp(args[0], "clr") == 0)
    {
        execl("usr/bin/clear", "clear", NULL); //in macos system("clear") doesnt work so we used execl
    }

    pid_t childpid;
    childpid = fork();

    if (childpid == 0) //if you are in child process
    {

        for (int ck = 0; args[ck] != NULL; ck++) // control all args until null
        {

            if (strcmp(args[ck], "<") == 0) // element is < so next element is file name
            {
                int fd;
                fd = open(args[ck + 1], O_RDONLY); //args[ck+1] keep filename //read only file
                dup2(fd, STDIN_FILENO);            //standart input from file
                close(fd);
                args[ck] = NULL; // for execl last function.
                continue;
            }
            if (strcmp(args[ck], ">>") == 0)
            {

                int fd;
                fd = open(args[ck + 1], (O_WRONLY | O_CREAT | O_APPEND), CREATE_MODE); //if file doesnt exist create file if its exist appen file
                dup2(fd, STDOUT_FILENO);                                               //standart output to file

                close(fd);
                args[ck] = NULL;
                continue;
            }
            if (strcmp(args[ck], "2>") == 0)
            {

                int fd;
                fd = open(args[ck + 1], (O_WRONLY | O_CREAT | O_TRUNC), CREATE_MODE); //if file doesnt exist  create file if its exist truncate that file
                dup2(fd, STDERR_FILENO);                                              //standart error to file

                close(fd);
                args[ck] = NULL;
                continue;
            }
            if (strcmp(args[ck], ">") == 0)
            {

                int fd;
                fd = open(args[ck + 1], (O_WRONLY | O_CREAT | O_TRUNC), CREATE_MODE); //if file doesnt exist  create file if its exist truncate that file
                dup2(fd, STDOUT_FILENO);

                close(fd);
                args[ck] = NULL;
                continue;
            }

            int i;
            for (i = 0; args[i] != NULL; i++)
                ;

            if (strcmp(args[i - 1], "&") == 0)
                args[i - 1] = NULL; //if last element is & change with NULL because this loop for only file i/o section
        }

        if (strcmp(path, "") == 0) //if path doesnt exist return "" value
        {
            perror("Please enter valid command\n");
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
              args[39], args[40], args[41]); //we dont know how many args entered so we used this type of function.
    }
    else //if you are not in chid process
    {
        if (background == 0)
        {
            waitpid(childpid, NULL, 0); //if no background process wait childprocess
        }

        else
        {
            waitpid(childpid, NULL, WNOHANG);    //if there is background process wait child
            pushpidlist(&pidlistHead, childpid); //and push pidlist to childs pid. 
            //For fg command , move background processes to foreground
        }
    }
}

void sighandler(int sig_num) ////////////DOESNT WORK \\\\\\\\\\\\\\\/
{
    signal(SIGTSTP, sighandler);
    pid_t pid;

    if ((pid = tcgetpgrp(STDOUT_FILENO)) < 0)
        perror("tcgetpgrp() error");
    else
        printf("the foreground process group id of stdout is %d\n", (int)pid);

    kill(pid, SIGSTOP);
    printf("Cannot execute Ctrl+Z\n");
} ////////////DOESNT WORK \\\\\\\\\\\\\\\/

int main(void)
{

    int ikk = 0;
    char path1[50];
    char argsfun1[50];
    char inputBuffer[MAX_LINE];
    int background;
    char *args[MAX_LINE / 2 + 1];

    const char *s = getenv("PATH"); //keep all paths, between :path1:path2:path3:
    int i = 0;
    char *p = strtok(s, ":"); //split with : character
    char *array[10];

    while (p != NULL)
    {
        array[i++] = p;

        p = strtok(NULL, ":"); //take all paths in array array
    }

    while (1)
    {

        fflush(stdout);
        background = 0;
    JMP: //for error checks return line
        printf("myshell: ");
        fflush(stdout);
        setup(inputBuffer, args, &background);

        //-------------ERROR CHECKS--------------\\\\\\\/
        if (args[0] == NULL)
        {
            printf("Please enter an input\n");
            goto JMP;
        }

        if (strcmp(args[0], "alias") == 0 && args[1] == NULL)
        {
            printf("Error please enter valid input\n");
            goto JMP;
        }

        if (strcmp(args[0], "unalias") == 0 && args[1] == NULL)
        {
            printf("Error please enter valid input\n");
            goto JMP;
        }

        if (strcmp(args[0], "alias") == 0 && strstr(args[1], "\"") == NULL && args[2] != NULL)
        {
            printf("Error: please enter valid input\n");
            goto JMP;
        }
        //-------------ERROR CHECKS--------------\\\\\\\/
        if (strcmp(args[0], "alias") == 0 && strcmp(args[1], "-l") != 0)
        { //if user enter alias but not alias -l
            if (args[2] == NULL)
            { //if user enter only alias throw error
                printf("Please enter valid command.\n");
                goto JMP;
            }
            char *subString;
            char argnew[50];
            argnew[0] = '\0';
            char argnewr[50];
            argnewr[0] = '\0';
            int k = 1;
            int j = 1;
            int z = 0;

            while (args[k] != NULL)//1 den basliyor cunku 0.indexde alias var
            {
                while (args[z] != NULL) //args 0dan itibaren butun commandleri birlestiriyor. Daha sonra tirnak icindeki ifadeyi alabilmek icin
                {
                    strcat(argnewr, args[z]);
                    strcat(argnewr, " ");//args arrayinin her elemeani arasina space koyuyor.
                    z++;
                }
                k++;
            }
            char *token;//alias "ls -l" list icin
            token = strtok(argnewr, "\"");//tirnak icindeki ifade ls -l
            token = strtok(NULL, "\"");
            pushAlias(&aliasHead, token, args[z - 1]);//token =ls-l (real value).
            continue;
        }//fake value icin z degerini sayiyoruz z son tirnak isaretinden sonraki indexi veriyor yani aliaslistesinde ls -l icin list degiskeni tutuluyor.

        if (strcmp(args[0], "unalias") == 0) //->unalias list
        {
            deleteAlias(&aliasHead, args[1]); //delete list in linkedlist
            continue;
        }

        NodeAlias *tempalias = aliasHead;
        int kk;

        while (tempalias != NULL)
        {
            kk = 0;
            if (strcmp(args[kk], tempalias->outquote) == 0)//eger kullanicinin girdigi command alias degerine sahipse
            {
                //args[kk]="ls -l" bu ifadeyi parcalamamiz lazim "ls" ve "-l" olarak. Exece gondermek icin
                strcpy(args[kk], tempalias->inquote);//o eleman yerine aliasdaki gercek degerini koy.
                char str[100];
                memset(str, '\0', sizeof(str));//str arrayini sıfırlamak için
                strcpy(str, args[kk]);//str="ls -l"
                char splitStrings[20][20];
                int j;
                int cnt;
                j = 0;
                cnt = 0;
                for (int i = 0; i <= (strlen(str)); i++)
                {
                    if (str[i] == ' ' || str[i] == '\0')// space ile ayirmak icin
                    {
                        splitStrings[cnt][j] = '\0';
                        cnt++; //for next word
                        j = 0; //for next word, init index to 0
                    }
                    else
                    {
                        splitStrings[cnt][j] = str[i];
                        j++;
                    }
                }//split strings arrayinde "ls" ve "-l" ayrildi. Bunu args arrayine ayni sekilde aktariyoruz
                int il;
                for (il = 0; il < cnt; il++)
                {
                    args[il] = malloc(sizeof(char *) * strlen(splitStrings[il]));//two dimension array oldugu icin bosaltmak icin malloc kullandik
                    strcpy(args[il], splitStrings[il]);
                }
                args[il] = NULL;
                continue;
            }
            kk++;
            tempalias = tempalias->next;//kullanicinin girdigi degerin alias olup olmadigini kontrol etmek icin butun listeyi geziyor.
        }

        if (args[0] == NULL) //if user didnt enter a command
        {
            printf("Please enter a value.\n");
            continue;
        }

        NodePidList *temppidlist = pidlistHead;
        if (strcmp(args[0], "fg") == 0) //if command is "fg"
        {
            while (temppidlist != NULL)
            {
                waitpid(temppidlist->pid, NULL, 0);            //wait background pid
                deletepidlist(&pidlistHead, temppidlist->pid); //delete this pid from linked list
                temppidlist = temppidlist->next;               //until all background proccess change to foreground process
            }

            continue;
        }

        if (strcmp(args[0], "alias") == 0 && strcmp(args[1], "-l") == 0) // if command is "alias -l"
        {
            if (aliasHead != NULL)
                printList(&aliasHead); //print alias list
            else
                printf("\n Alias list empty.\n"); //if list is empty print this statemenet

            continue;
        }

        getpath(args, array, path1);
        execfunc(background, path1, args);
        //signal(SIGTSTP, sighandler); //FOR CTRL Z COMMAND BUT DOESNT WORK
        goto JMP;
    }
}
