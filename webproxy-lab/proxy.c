#include <stdio.h>
#include "csapp.h"

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr =
    "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 "
    "Firefox/10.0.3\r\n";

void parse(int connfd);
int parse_uri(char *uri, char *filename, char *cgiargs);
void clienterror(int connfd, char *cause, char *errnum, char *shortmsg, char *longmsg);
void read_requesthdrs(rio_t *rp);
void parse_url(char *uri_pass1, char *host, char *port, char *uri);
void *thread(void *vargp);

int main(int argc, char **argv)
{
  printf("%s", user_agent_hdr);
  char hostname[MAXLINE], port[MAXLINE], fixed_port[MAXLINE];
  int listenfd, clientfd, *connfdp;
  socklen_t clientlen;
  struct sockaddr_storage clientaddr;
  pthread_t tid;

  // 포트 자동 생성해야 함.33404
  // strcpy(fixed_port, "33404");
  // printf("\nfixed_port: %s\n", fixed_port);
  listenfd = Open_listenfd(argv[1]);
  while (1)
  {
    clientlen = sizeof(clientaddr);
    connfdp = Malloc(sizeof(int));
    *connfdp = Accept(listenfd, (SA *)&clientaddr,
                    &clientlen);
    Getnameinfo((SA *)&clientaddr, clientlen, hostname, MAXLINE, port, MAXLINE, 0);
    Pthread_create(&tid, NULL, thread, connfdp);
    // doit(*connfdp);
    // Close(*connfdp);
  }

  return 0;
}

void *thread(void *vargp)
{
  int connfd = *((int *)vargp);
  Pthread_detach(pthread_self());
  // Further, we must be careful to free the memory block that was allocated by the main thread
  Free(vargp);
  doit(connfd); 
  Close(connfd);
  return NULL;
}

void doit(int connfd)
{
  int is_static;
  char buf[MAXLINE], method[MAXLINE],
      uri_pass1[MAXLINE], host[MAXLINE], uri[MAXLINE], port[MAXLINE],
      version[MAXLINE];
  char filename[MAXLINE], cgiargs[MAXLINE];
  char *delim;
  struct stat sbuf;
  rio_t rio;

  int clientfd;
  char req_buf[MAXLINE];
  char res_buf[MAX_OBJECT_SIZE];

  Rio_readinitb(&rio, connfd);
  Rio_readlineb(&rio, buf, MAXLINE);
  printf("Request headers:\n");
  printf("%s", buf);
  if (sscanf(buf, "%s %s %s", method, uri_pass1, version) != 3)
  {
    printf("invalid request header form\n");
    return;
  }

  parse_url(uri_pass1, host, port, uri);

  if (strcasecmp(method, "GET"))
  {
    clienterror(connfd, method, "501", "Not implemented", "Tiny does not implement this method");
    return;
  }
  read_requesthdrs(&rio);
  is_static = parse_uri(uri, filename, cgiargs);
  
  printf("\n\nmethod: %s\n", method);
  printf("host: %s\n", host);
  printf("uri: %s\n", uri);
  printf("port: %s\n", port);
  printf("version: %s\n", version);
  printf("file name parsed: %s\n", filename);
  // 대상 서버로 요청 전달하는 부분. 에코 클라이언트 서버를 좀 참고해보자.
  clientfd = Open_clientfd(host, port);
  if (clientfd < 0) {
    clienterror(connfd, host, "502", "Bad Gateway", "Failed to connect to the target server");
    return;
  }
  printf("clientfd: %d\n", clientfd);
  
  // header 보내버리기
  Rio_readinitb(&rio, clientfd);
  sprintf(req_buf, "%s %s %s\r\nHost: %s\r\n%s\r\n", method, uri, "HTTP/1.0", host, user_agent_hdr);
  Rio_writen(clientfd, req_buf, strlen(req_buf));
  
  // Read response from server and forward to client, handling binary data
  ssize_t n;
  while ((n = Rio_readn(clientfd, res_buf, MAX_OBJECT_SIZE)) > 0) {
    Rio_writen(connfd, res_buf, n);
    if (n < MAX_OBJECT_SIZE) break; // likely end of response
  }
  // Rio_writen(connfd, res_buf, strlen(res_buf));
  // Fputs(buf, stdout);

  // if (is_static)
  // {
  //   // Finally, if the request is for static content, we verify that the file is a regular file and that we have read permission.
  //   // 여기서 파일 존재 여부 체크 로직을 추가해야 하는 구나.
  //   if (!(S_ISREG(sbuf.st_mode)) || !(S_IRUSR & sbuf.st_mode))
  //   {
  //     clienterror(connfd, filename, "403", "Forbidden", "Tiny couldn't read the file");
  //     return;
  //   }
  //   // If so, we serve the static content to the client.
  //   // serve_static(connfd, filename, sbuf.st_size);
  //   printf("serving static file: %s", filename);
  // }
  // // Similarly, if the request is for dynamic content, we verify that the file is executable, and, if so, we go ahead and serve the dynamic content.
  // else
  // {
  //   // 마찬가지.
  //   if (!(S_ISREG(sbuf.st_mode)) || !(S_IXUSR & sbuf.st_mode))
  //   {
  //     clienterror(connfd, filename, "403", "Forbidden", "Tiny couldn't run the CGI program");
  //     return;
  //   }
  //   // serve_dynamic(connfd, filename, cgiargs);
  //   printf("serving dynamic file: %s", filename);
  // }
}

int parse_uri(char *uri, char *filename, char *cgiargs)
{
  char *ptr;

  // If the request is for static content, we clear the CGI argument string and
  // then convert the URI into a relative Linux pathname such as ./index.htm.
  // If the URI ends with a '/' character, then we append the default filename.
  if (!strstr(uri, "cgi-bin"))
  {
    strcpy(cgiargs, "");
    strcpy(filename, ".");
    strcat(filename, uri);
    if (uri[strlen(uri) - 1] == '/')
      strcat(filename, "home.html");
    return 1;
  }
  // On the other hand, if the request is for dynamic content,
  // we extract any CGI arguments and
  // convert the remaining portion of the URI to a relative Linux filename
  else
  {
    ptr = index(uri, '?');
    // It parses the URI into a filename and an optional CGI argument string.
    if (ptr)
    {
      strcpy(cgiargs, ptr + 1);
      *ptr = '\0';
    }
    else
      strcpy(cgiargs, "");
    strcpy(filename, ".");
    strcat(filename, uri);
    return 0;
  }
}
void clienterror(int connfd, char *cause, char *errnum, char *shortmsg, char *longmsg)
{
  char buf[MAXLINE], body[MAXBUF];

  // build the HTTP response body
  sprintf(body, "<html><title>Tiny Error</title>");
  sprintf(body, "%s,body bgcolor="
                "ffffff"
                ">\r\n",
          body);
  sprintf(body, "%s%s: %s\r\n", body, errnum, shortmsg);
  sprintf(body, "%s<p>%s: %s\r\n", body, longmsg, cause);
  sprintf(body, "%s<hr><em>The Tiny Web server</em>\r\n", body);

  sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg);
  Rio_writen(connfd, buf, strlen(buf));
  sprintf(buf, "Content-type: text/html\r\n");
  Rio_writen(connfd, buf, strlen(buf));
  sprintf(buf, "Content-length: %d\r\n\r\n", (int)strlen(body));
  Rio_writen(connfd, buf, strlen(buf));
  Rio_writen(connfd, body, strlen(body));
}

void read_requesthdrs(rio_t *rp)
{
  char buf[MAXLINE];

  Rio_readlineb(rp, buf, MAXLINE);
  while (strcmp(buf, "\r\n"))
  {
    Rio_readlineb(rp, buf, MAXLINE);
    printf("%s", buf);
  }
  return;
}

void parse_url(char *uri_pass1, char *host, char *port, char *uri)
{
  char *hostbegin, *hostend, *portbegin, *uribegin;
  int hostlen;

  // Assume uri_pass1 starts with "http://"
  if (strncasecmp(uri_pass1, "http://", 7) == 0)
    hostbegin = uri_pass1 + 7;
  else
    hostbegin = uri_pass1;

  // Find the first '/' after host
  uribegin = strchr(hostbegin, '/');
  if (uribegin) {
    strcpy(uri, uribegin);
    hostend = uribegin;
  } else {
    uri[0] = '/';
    uri[1] = '\0';
    hostend = hostbegin + strlen(hostbegin);
  }

  // Look for port
  portbegin = strchr(hostbegin, ':');
  if (portbegin && portbegin < hostend) {
    hostlen = portbegin - hostbegin;
    strncpy(host, hostbegin, hostlen);
    host[hostlen] = '\0';
    portbegin++;
    int portlen = hostend - portbegin;
    strncpy(port, portbegin, portlen);
    port[portlen] = '\0';
  } else {
    hostlen = hostend - hostbegin;
    strncpy(host, hostbegin, hostlen);
    host[hostlen] = '\0';
    strcpy(port, "80");
  }
}
