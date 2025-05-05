#include "csapp.h"
#define N 10

/* 
# Summary
Because the waitpid function is somewhat complicated, it is helpful to look at a few examples.
The code is a program that uses waitpid to wait, in no particular order, for all of tis N children to terminate.
# Comment: result is unexpectable.  
When we run the program on our Linux system, it produces the following output
Notice that the program reaps its children in no particular order.
The order that they were reaped is a property of this specific computer system. 
On another system, or even another execution on the same system, the two children might have been reaped in the opposite order.
This is an example of the nondeterministic behavior that can make reasoning about concurrency so difficult.
Either of the two possible outcomes is equally correct, and as a programmer you may never assume that one outcome will always occur, 
no matter how unlikely the other outcome appears to be. 
The only correct assumption is that each possible outcome is equally likely.
 */
/**
int main()
{
    int status, i;
    pid_t pid;

    // parent creates N children
    for ( i = 0; i < N; i++)
    {
        // The parent creates each of the N children, 
        // and in line 12, each child exits with a unique exit status.
        // Before moving on, make sure you understand why line 12 is executed by each of the children,
        //  but not the parent
        if ((pid = Fork()) == 0) exit(100+i);
    }

    // Parent reaps N children in no particular order
    // In line 15, the parent waits for 
    // all of its children to terminate by using waitpid as the test condition of a while loop. 
    // Because the first argument is -1, 
    // the call to waitpid blocks until an arbitrary child has terminated.
    // As each child terminates, the call to waitpid returns with the nonzero PID of that child.
    while ((pid = waitpid(-1, &status, 0))> 0){ // When all of the children have been reaped, the next call to waitpid returns -1 and sets errno to ECHILD.
        // Line 16 checks the exit status of the child. 
        if (WIFEXITED(status))
            // If the child terminated normally - in this case, by calling the exit function - 
            // then the parent extracts the exit status and prints it on stdout.
            printf("child %d terminated normally with exit status = %d\n", pid, WEXITSTATUS(status));
        else
            printf("child %d terminated abnormally\n", pid);
    }

    // The only normal termination is if there are no more children
    // Line 24 checks that the waitpid function terminated normally, and prints an error message otherwise.
    if (errno != ECHILD)
        unix_error("waitpid error");
    exit(0);
}
 */
/**
int main()
{
    pid_t pid;
    int x = 1;

    pid = Fork();
    if(pid == 0) {
        printf("child : x=%d\n", ++x);
        exit(0);
    }

    printf("parent: x=%d\n", --x);
    exit(0); // 여기서 x가 의미하는 바가 뭐지? 성공적으로 종료되었음을 나타내는 종료 상태.
}
 */
// 매우 유용한 예제
/**
// int main()
{
    Fork();//  << x
    Fork(); // <== 0
    printf("hello\n");
    exit(0);
}
 */

/* // Practice 8.6
int main(int argc, char **argv, char **envp)
{
    printf("Commandline arguments: \n");
    for (int i = 0; argv[i]; i++)
    {
        printf("argv[%d]: %s\n", i, argv[i]);
    }
    printf("\nEnvironment variables:\n");
    for (int i = 0; envp[i]; i++)
    {
        printf("envp[%d]: %s\n", i, envp[i]);
    }
} */


#include "csapp.h"
#define MAXARGS 128

// Function prototypes
void eval(char *cmdline);
int parseline(char *buf, char **argv);
int builtin_command(char **argv);

int main()
{
    char cmdline[MAXLINE];
    while (1)
    {
        // Read
        printf("> ");
        Fgets(cmdline, MAXLINE, stdin);
        if (feof(stdin)) exit(0);
        // evaluate
        eval(cmdline);

    }
}

// eval - Evaluate a command line
void eval(char *cmdline)
{
    char *argv[MAXARGS]; // Argument list execve()
    char buf[MAXLINE]; // holds modified command line
    int bg; // should the job ron in bg or fg?
    pid_t pid; // process id

    strcpy(buf, cmdline);
    bg = parseline(buf, argv);
    if (argv[0] == NULL)
        return;
    
    if (!builtin_command(argv)){
        if ((pid = Fork()) == 0)
        {
            if (execve(argv[0], argv, environ) < 0)
            {
                printf("%s: Command not found. \n", argv[0]);
                exit(0);
            }
        }
        if (!bg)
        {
            int status;
            if (waitpid(pid, &status, 0) < 0)
                unix_error("waitfg: waitpid error");
        }
        else
            printf("%d %s", pid, cmdline);
    }
    return;
}

// If first arg is a builtin command, run it and return true
int builtin_command(char **argv)
{
    if (!strcmp(argv[0], "quit")) // quit command
        exit(0);
    if(!strcmp(argv[0], "&")) // Ignore singleton
        return 1;
    return 0; // Not a builtin command
}

int parseline(char *buf, char **argv)
{
    char *delim; // Points to first space delimiter
    int argc; // number of args
    int bg; // background job?

    buf[strlen(buf) - 1] = ' '; // replace trailing '\n' with space
    while (*buf && (*buf == ' ')) // ignore leading spaces
        buf++;
    
    // build the argv list
    argc = 0;
    while ((delim = strchr(buf, ' ')))
    {
        argv[argc++] = buf;
        *delim = '\0';
        buf = delim + 1;
        while(*buf && (*buf == ' '))
            buf++;
    }
    argv[argc] = NULL;
    if(argc == 0) return 1;

    // should the job run in the background?
    if ((bg = (*argv[argc-1] == '&')) != 0)
        argv[--argc] = NULL;
    return bg;
}