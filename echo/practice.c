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
/*

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
} */

/*

void echo(int connfd);
void sigchld_handler(int sig);

void echo(int connfd)
{
    size_t n; // 에코 실행을 위해 문자열의 사이즈를 입력 받음.
    char usrbuf[MAXLINE]; // 버퍼 문자열 배열. 옆과 같이 정의돼 있음. #define MAXLINE 8192
    rio_t rio; // robust i/o. 이것 자체는 구조체. 어렵진 않음.

    Rio_readinitb(&rio, connfd);
    while((n = Rio_readlineb(&rio, usrbuf, MAXLINE)) != 0)
    {
        printf("server received %d bytes\n", (int)n);
        Rio_writen(connfd, usrbuf, n);
    }
}

void sigchld_handler(int sig)
{
    while(waitpid(-1, 0, WNOHANG) > 0)
    ;
    return;
}

int main(int argc, char **argv)
{
    int listenfd, connfd;
    socklen_t clientlen;
    struct sockaddr_storage clientaddr;

    if (argc != 2)
    {
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        exit(0);
    }
    Signal(SIGCHLD, sigchld_handler);
    listenfd = Open_listenfd(argv[1]);
    while (1)
    {
        clientlen = sizeof(struct sockaddr_storage);
        connfd = Accept(listenfd, (SA *) &clientaddr, &clientlen);
        if (Fork() == 0)
        {
            Close(listenfd); // Child closes its listening socket
            echo(connfd);
            Close(connfd);
            exit(0);
        }
        Close(connfd);

    }


}

 */

/*
#define N 2

void *thread(void *vargp);

// A global variable is any variable declared outside of a function.
// At run time, the read/write area of virtual memory contains exactly one instance of each
// global variable that can be referenced by any thread.
// For example, the global ptr variable declared in line 5 has one run-time instance in the
// read/write area of virtual memory.
// When there is only one instance of a variable, we will denote the instance by simply using the
// variable name - in this case, ptr.

char **ptr;

int main()
{
    int i;
    pthread_t tid;
    char *msgs[N] = {
        "Hello from foo",
        "Hello from bar"
    };

    ptr = msgs;
    for ( i = 0; i < N; i++)
        Pthread_create(&tid, NULL, thread, (void *)i);
    Pthread_exit(NULL);
}

void *thread(void *vargp)
{

    // Local automatic variables
    // A local automatic variable is one that is declared inside a function without the static attribute.
    // At run time, each thread's stack contains its own instances of any local automatic variables.
    // This is true even if multiple threads execute the same thread routine.
    // For example, there is one instance of the local variable tid, and it resides on the stack of the
    // main thread.
    // We will denote this instance as tid.m
    // As another example, there are two instances of the local variable myid, one instance on the stack
    // of peer thread 1. We will denote these instances as myid.p0 and myid.p1 respectively.

    int myid = (int)vargp;

    // Local static variables
    // A local static variable is one that is declared inside a function with the static attribute.
    // As with global variables, the read/write area of virtual memory contains exactly one instance of
    // each local static variable declared in a program.
    // For example, even though each peer thread in our example program declares cnt in line 25,
    // at run time there is only one instance of cnt residing in the read/write area of virtual memory.
    // Each peer thread reads and writes this instanc  e.

    static int cnt = 0;
    printf("[%d]: %s (cnt=%d)\n", myid, ptr[myid], ++cnt);
    return NULL;
}

 */


void *thread(void *vargp); // Thread routine prototype

// Global shared variable
volatile long cnt = 0; // Counter
sem_t mutex;

int main(int argc, char **argv)
{
    long niters;
    pthread_t tid1, tid2;
    Sem_init(&mutex, 0, 1); // mutex = 1

    // Check input argument
    if (argc != 2)
    {
        printf("usage: %s <niters>\n", argv[0]);
        exit(0);
    }

    niters = atoi(argv[1]);
    printf("niters: %ld\n", niters);

    // Create threads and wait for them to finish
    Pthread_create(&tid1, NULL, thread, &niters);
    Pthread_create(&tid2, NULL, thread, &niters);
    Pthread_join(tid1, NULL);
    Pthread_join(tid2, NULL);

    // Check result
    if (cnt != (2 * niters))
        printf("BOOM! cnt = %ld\n", cnt);
    else
        printf("OK cnt=%ld\n", cnt);
    exit(0);
}
// Thread routine
void *thread(void *vargp)
{
    long i, niters = *((long *)vargp);
    for(i = 0; i < niters; i++)
    {
        P(&mutex);
        cnt++;
        V(&mutex);
    }
    return NULL;
}


/*
#include "sbuf.h"

// Create an empty, bounded, shared FIFO buffer with n slots
void sbuf_init(sbuf_t *sp, int n)
{
    sp->buf = Calloc(n, sizeof(int));
    sp->n = n; // buffer holds max of n items
    sp->front = sp->rear = 0; // Empty buffer if front == rear
    Sem_init(&sp->mutex, 0, 1); // Binary semaphore for locking
    Sem_init(&sp->slots, 0, n); // Initially buf has n empty slots
    Sem_init(&sp->items, 0, 0); // Initially buf has zero data items.
}

// Clean up buffer sp
void sbuf_deinit(sbuf_t *sp)
{
    Free(sp->buf);
}

// insert item onto the rear of shared buffer sp
void sbuf_insert(sbuf_t *sp, int item)
{
    P(&sp->slots); // Wait for available slot
    P(&sp->mutex); // Lock the buffer
    sp->buf[(++sp->rear)%(sp->n)] = item; // Insert the item
    V(&sp->mutex); // Unlock the buffer
    V(&sp->items); // Announce available item
}

// Remove and return the first item from buffer sp
int sbuf_remove(sbuf_t *sp)
{
    int item;
    P(&sp->items); // Wait for available item
    P(&sp->mutex); // Lock the buffer
    item = sp->buf[(++sp->front)%(sp->n)]; // Remove the item
    V(&sp->mutex); // Unlock the buffer
    V(&sp->slots); // Announce available slot
    return item;
}
 */
/* 

// Solution to the first readers-writers problem

// Global variables
int readcnt; // Initially = 0
// The w semaphore controls access to the critical sections that access the shared
// object.
// The mutex semaphore protects access to the shared readcnt variable,
// which counts the number of readers currently in the critical section.
sem_t mutex, w; // Both initially = 1

void reader(void)
{
    while (1)
    {
        //  On the other hand, only the first reader to enter the critical section locks
        // w, and only the last reader to leave the critical section unlocks it.
        P(&mutex);
        readcnt++;
        if (readcnt == 1) // First in
                          // The w mutex is ignored by readers who enter and leave while other readers are present.
                          //  This means that as long as a single reader holds the w mutex, an unbounded number of
                          // readers can enter the critical section unimpeded.
            P(&w);
        V(&mutex);

        // Critical section
        // Reading happens

        P(&mutex);
        readcnt--;
        if (readcnt == 0) // Last out
            V(&w);
        V(&mutex);
    }
}

void writer(void)
{
    while (1)
    {
        // A writer locks the w mutex each time it enters the critical section and
        //    unlocks it each time it leaves.
        // This guarantees that there is at most one writer in the critical section at any point
        // in time.
        P(&w);

        // Critical section
        // Writing happens

        V(&w);
    }
}
 */