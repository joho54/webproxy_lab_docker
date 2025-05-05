#include "csapp.h"
void echo(int connfd);

/* 
The code for a concurrent echo server based on processes. 
The echo function called in line 29 comes from figure 11.22.
There are several important points to make about this server:
*/


// First, servers typically run for long periods of time, so we must include a SIGCHILD handler that reaps zombie children.
// Since SIGCHILD signals are blocked while the SIGCHILD handler is executing, and since Linux signals are not queued, 
// the SIGCHLD handler must be prepared to reap multiple zombie children
// comment: sigchld_handler는 다른 자식 프로세스가 끝날 때까지 부모를 대기시키기 위해 있는 건가?
void sigchld_handler(int sig)
{
    while (waitpid(-1, 0, WNOHANG) > 0)
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
    Signal(SIGCHILD, sigchld_handler);
    listenfd = Open_listenfd(argv[1]);
    while (1)
    {
        clientlen = sizeof(struct sockaddr_storage);
        connfd = Accept(listenfd, (SA *) &clientaddr, &clientlen);
        if (Fork() == 0)
        {
            Close(listenfd); // child closes its listening socket
            echo(connfd); // child sservices client
            Close(connfd); // child closes connection with client
            exit(0);
        }
        // Second, the parent and the child must close their respective copies of connfd. 
        // As we have mentioned, this is especially important for the parent, which must close its copy of the connected descriptor to
        // avoid a memory leak.
        // comment: 파일 디스크립터를 닫지 않으면 메모리 누수가 난다는 말은, 파일 디스크립터 테이블에 있는 파일들도 기본적으로는 다 메모리에 있다는 건가? 그러면 왜 따로
        // write같은 함수로 파일 디스크립터의 파일을 메모리로 복사하는 함수 같은게 존재하는 거지?
        Close(connfd); //Parent closes connected socket ( important! )
        
    }
    
}