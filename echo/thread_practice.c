#include "csapp.h"
/* 
void *thread(void *vargp);

int main()
{
    pthread_t tid;
    Pthread_create(&tid, NULL, thread, NULL);
    Pthread_join(tid, NULL);
    exit(0);
}

void *thread(void *vargp) // Thread routine
{
    printf("Hello, world!\n");
    return NULL;
}

 */

/* 
The code for a concurrent echo server based on threads. The overall structure is similar to the process-based design.
The main thread repeatedly waits for a connection request and then creates a peer thread to handle the request.
While the code looks simple, there are a couple of general and somewhat subtle issues we need to look at more closely.
The first issue is how to pass the connected descriptor to the peer thread when we call pthread_create.
The obvious approach is to pass a pointer to the descriptor, as in the following.
```
connfd = Accept(listenfd, (SA *) &clientaddr, &clientlen);
Pthread_create(&tid, NULL, thread, &connfd);
```
Then we have the peer thread dereference the pointer and assign it to a local variable, as follows
```
void *thread(void *vargp){
    int connfd = *((int *)vargp);
}
```

This would be wrong, however, because it introduces a race between the assignment statement in the peer thread 
and the accept statement in the main thread. If the assignment statement completes before the next accept, 
then the local connfd variable in the peer thread gets the correct descriptor value. (which is not a problem)
However!, if the assignment completes after the accept, then the local connfd variable 
in the peer thread gets the descriptor number of the next connection.
The unhappy result is that two threads are now performing input and output on the same descriptor.

Another issue is avoiding memory leaks in the thread routine.


*/

void echo(int connfd);
void *thread(void *vargp);

void echo(int connfd)
{
    size_t n; // 에코 실행을 위해 문자열의 사이즈를 입력 받음.
    char usrbuf[MAXLINE]; // 버퍼 문자열 배열. 옆과 같이 정의돼 있음. #define MAXLINE 8192  /* Max text line length */
    rio_t rio; // robust i/o. 이것 자체는 구조체. 어렵진 않음.

    Rio_readinitb(&rio, connfd);
    while((n = Rio_readlineb(&rio, usrbuf, MAXLINE)) != 0)
    {
        printf("server received %d bytes\n", (int)n);
        Rio_writen(connfd, usrbuf, n);
    }
}

int main(int argc, char **argv)
{
    int listenfd, *connfdp;
    socklen_t clientlen;
    struct sockaddr_storage clientaddr;
    pthread_t tid;

    if (argc != 2)
    {
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        exit(0);
    }

    listenfd = Open_listenfd(argv[1]);

    while (1)
    {
        clientlen = sizeof(struct sockaddr_storage);
        // In order to avoid the potentially deadly race, we mut assign each connected descriptor returned by accept 
        // to it0xaaaaaaac12f0s own  dynamically allocated memory block, as shown in line 21-22. 
        connfdp = Malloc(sizeof(int));
        *connfdp = Accept(listenfd, (SA *) &clientaddr, &clientlen);
        Pthread_create(&tid, NULL, thread, connfdp);
    }
}

// Thread routine
void *thread(void *vargp)
{
    int connfd = *((int *)vargp);
    // Since we are not explicitly reaping threads, we must detach each thread so that
    // its memory resources will be reclaimed when it terminates.
    Pthread_detach(pthread_self());
    // Further, we must be careful to free the memory block that was allocated by the main thread
    Free(vargp);
    echo(connfd);
    Close(connfd);
    return NULL;
}
