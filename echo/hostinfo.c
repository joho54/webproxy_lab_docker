#include "csapp.h"

int main(int argc, char **argv)
{
    struct addrinfo *p, *listp, hints;
    char host_buf[MAXLINE];
    int rc, flags;

    if (argc != 2)
    {
        fprintf(stderr, "usave: %s <domain name>\n", argv[0]);
        exit(0);
    }
    // Get a list of addrinfo records
    memset(&hints, 0, sizeof(struct addrinfo));
    printf("hints init: %p\n", (&hints));
    hints.ai_family = AF_INET; // IP4 only
    hints.ai_socktype = SOCK_STREAM; // Connections only
    printf("hints.ai_family: %d\n", hints.ai_family);
    printf("hints.ai_socktype: %d\n", hints.ai_socktype);
    if ((rc = getaddrinfo(argv[1], NULL, &hints, &listp)) != 0)
    {
        fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(rc));
        exit(1);
    }
    // walk the list and display each IP address
    flags = NI_NUMERICHOST; // Display address string instead of domain name.
    for ( p = listp; p ; p = p->ai_next)
    {
        Getnameinfo(p->ai_addr, p->ai_addrlen, host_buf, MAXLINE, NULL, 0, flags);
        printf("%s\n", host_buf);
    }

    Freeaddrinfo(listp);
    exit(0);
    
}