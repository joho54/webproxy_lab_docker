#include "csapp.h"
void echo(int connfd);
void sigchld_handler(int sig);


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