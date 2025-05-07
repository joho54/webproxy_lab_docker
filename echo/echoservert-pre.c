#include "csapp.h"
#include "sbuf.h"

#define NTHREADS 4
#define SBUFSIZE 2

void echo_cnt(int connfd);
void *thread(void *vargp);
static void init_echo_cnt(void);
void echo_cnt(int connfd);

sbuf_t sbuf; // shared buffer of connected descriptors



/*
The code shows how we would use the SBUF package to implement a prethreaded concurrent echo server.
*/

int main(int argc, char **argv)
{
    int i, listenfd, connfd;
    socklen_t clientlen;
    struct sockaddr_storage clientaddr;
    pthread_t tid;

    if (argc != 2)
    {
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        exit(0);
    }

    listenfd = Open_listenfd(argv[1]);
    // After initializing buffer sbuf (line 24)
    sbuf_init(&sbuf, SBUFSIZE);
    // the main thread creates the set of worker threads.
    for (i = 0; i < NTHREADS; i++)
        Pthread_create(&tid, NULL, thread, NULL);

    while (1)
    {
        // Then it enters the infinite server loop, accepting connection requests and inserting the resulting
        // connected descriptors in sbuf.
        clientlen = sizeof(struct sockaddr_storage);
        connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);
        sbuf_insert(&sbuf, connfd); // Insert connfd in buffer
    }
}

// Each worker thread has a very simple behavior.
// It waits until it is able to remove a connected descriptor from the buffer and
// then calls the echo_cnt function to echo client input.
void *thread(void *argp)
{
    Pthread_detach(pthread_self());
    while (1)
    {
        int connfd = sbuf_remove(&sbuf); // Remove connfd from buffer 버퍼 먹어치우는 함수
        echo_cnt(connfd);                // Service client
        Close(connfd);
    }
}

// In our case, we need to initialize the byte_cnt counter and the mutex semaphore.
static int byte_cnt; // Byte counter
static sem_t mutex;  // and the mutex that protects it

static void init_echo_cnt(void)
{
    Sem_init(&mutex, 0, 1);
    byte_cnt = 0;
}

// The echo_cnt function is a version of echo function from before that records the cumulative number of bytes
// received from all clients in a global variable called byte_cnt.
// This is interesting code to study because it shows you a general technique for initializing packages that are
// called from thread routines.
void echo_cnt(int connfd)
{
    int n;
    char buf[MAXLINE];
    rio_t rio;
    static pthread_once_t once = PTHREAD_ONCE_INIT;
    // One approach, which we used for the SBUF and RIO packages,
    // is to require the main thread to explicitly call an initialization function.
    // Another approach, shown here, uses the pthread_once function to call the initialization function
    // the first time some thread calls the echo_cnt function.
    // The advantage of this approach is that it makes a call to pthread_once,
    // which most times does nothing useful.
    Pthread_once(&once, init_echo_cnt);
    Rio_readinitb(&rio, connfd);
    // Once the package is initialized,
    // the echo_cnt function initializes the RIO buffered I/O package and then
    // echoes each text line that is received from the client.
    while ((n = Rio_readlineb(&rio, buf, MAXLINE)) != 0)
    {
        // Notice that the accesses to the shared byte_cnt variable
        // in lines 23-25 are protected by P and V operation.
        P(&mutex);
        byte_cnt += n;
        printf("server received %d (%d total) bytes on fd %d\n", n, byte_cnt, connfd);
        V(&mutex);
        Rio_writen(connfd, buf, n);
    }
}
