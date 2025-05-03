#include "csapp.h"

void echo(int connfd, uint64_t id);

void echo(int connfd, uint64_t id)
{
    size_t n; // 에코 실행을 위해 문자열의 사이즈를 입력 받음.
    char usrbuf[MAXLINE]; // 버퍼 문자열 배열. 옆과 같이 정의돼 있음. #define MAXLINE 8192  /* Max text line length */
    rio_t rio; // robust i/o. 이것 자체는 구조체. 어렵진 않음.

    Rio_readinitb(&rio, connfd);
    while((n = Rio_readlineb(&rio, usrbuf, MAXLINE)) != 0)
    {
        printf("server received %d bytes\n", (int)n);
        INFO("[#%llu] read %zuB", (unsigned long long)id, n);
        Rio_writen(connfd, usrbuf, n);
        INFO("[#%llu] write %zuB", (unsigned long long)id, n);

    }
    INFO("[#%llu] closed", (unsigned long long)id, n);
}

int main(int argc, char **argv)
{
    int listenfd, connfd;
    socklen_t clientlen;
    struct sockaddr_storage clientaddr; // enough space for any address
    char client_hostname[MAXLINE], client_port[MAXLINE];

    if (argc != 2)
    {
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        exit(0);
    }

    listenfd = Open_listenfd(argv[1]); 
    INFO("server start, listen on %s", argv[1]); 

    // 연결 태그용 시퀀스
    uint64_t conn_seq = 0; 
    
    while (1) // 여기에 왜 루프가 있지?
    {
        clientlen = sizeof(struct sockaddr_storage); // 
        connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);
        Getnameinfo((SA *) &clientaddr, clientlen, client_hostname, MAXLINE, client_port, MAXLINE, 0);
        uint64_t id = ++conn_seq;
        INFO("[#%llu %s:%s] accept (fd=%d)", (unsigned long long) id, client_hostname, client_port, connfd);
        echo(connfd, id);
        Close(connfd);
    }
    exit(0);  
}
