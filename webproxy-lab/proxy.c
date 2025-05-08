#include <stdio.h>
#include "csapp.h"

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr =
    "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 "
    "Firefox/10.0.3\r\n";

typedef struct
{
  char uri[MAXLINE];               // key
  char cache_buf[MAX_OBJECT_SIZE]; // value
  int entry_size;                  // size of an entry
  void *prev;                      // prev pointer
  void *next;                      // next pointer
  sem_t mutex;
} cache_entry;

typedef struct
{
  void *head;     // pointer to head elem.
  int cache_size; // total cache size.
} cache_list;

void parse(int connfd);
int parse_uri(char *uri, char *filename, char *cgiargs);
void clienterror(int connfd, char *cause, char *errnum, char *shortmsg, char *longmsg);
void read_requesthdrs(rio_t *rp);
void parse_url(char *uri_pass1, char *host, char *port, char *uri);
void *thread(void *vargp);
int read_cache(char *key_uri, cache_entry *rcp);
int insert_cache(char *uri, char *res_buf, int cache_size);

static cache_list *cl;

int main(int argc, char **argv)
{
  printf("%s", user_agent_hdr);
  char hostname[MAXLINE], port[MAXLINE], fixed_port[MAXLINE];
  int listenfd, clientfd, *connfdp;
  socklen_t clientlen;
  struct sockaddr_storage clientaddr;
  pthread_t tid;

  // cache 리스트 초기화
  cl = Malloc(sizeof(cache_list));
  cl->head = NULL;
  cl->cache_size = 0;

  listenfd = Open_listenfd(argv[1]);
  while (1)
  {
    clientlen = sizeof(clientaddr);
    connfdp = Malloc(sizeof(int));
    *connfdp = Accept(listenfd, (SA *)&clientaddr,
                      &clientlen);
    Getnameinfo((SA *)&clientaddr, clientlen, hostname, MAXLINE, port, MAXLINE, 0);
    Pthread_create(&tid, NULL, thread, connfdp);
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
  cache_entry *rcp = Malloc(sizeof(cache_entry));
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

  if (read_cache(uri_pass1, rcp))
  {
    printf("cache HIT!\n");
    Rio_writen(connfd, rcp->cache_buf, rcp->entry_size);
    Free(rcp);
  }
  else // cache miss
  {
    printf("cache MISS!\n");

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
    if (clientfd < 0)
    {
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
    // first we read it and determine what to do
    int is_caching = ((n = Rio_readn(clientfd, res_buf, MAX_OBJECT_SIZE)) < MAX_OBJECT_SIZE);

    // whatever the size is, we flush it out first.
    char cache_tmp[MAX_OBJECT_SIZE];
    strncpy(cache_tmp, res_buf, MAX_OBJECT_SIZE - 1);
    cache_tmp[MAX_OBJECT_SIZE - 1] = '\0';

    Rio_writen(connfd, res_buf, n);

    if (is_caching)
    {
      insert_cache(uri_pass1, cache_tmp, n);
    }
    else // else read until end
    {
      while ((n = Rio_readn(clientfd, res_buf, MAX_OBJECT_SIZE)) > 0)
      {
        Rio_writen(connfd, res_buf, n);
        if (n < MAX_OBJECT_SIZE)
          break; // likely end of response
      }
    }
  }
}

int read_cache(char *key_uri, cache_entry *rcp)
{
  cache_entry *cp;
  for (cp = cl->head; cp; cp = cp->next)
  {
    if (!strcmp(key_uri, cp->uri)) // found identical uri
    {
      memcpy(rcp, cp, sizeof(cache_entry));

      // move the entry to the forward
      return 1;
    }
  }
  return 0;
}

int insert_cache(char *uri, char *res_buf, int cache_size)
{
  // First we insert it on head "Brute forcefully"
  cache_entry *nc = Malloc(sizeof(cache_entry));
  sem_init(&nc->mutex, 0, 1);
  P(&nc->mutex);
  strncpy(nc->uri, uri, MAXLINE - 1);
  nc->uri[MAXLINE - 1] = '\0';
  strncpy(nc->cache_buf, res_buf, MAXLINE - 1);
  nc->cache_buf[MAXLINE - 1] = '\0';
  // strcpy(nc->cache_buf, res_buf);
  nc->entry_size = cache_size;
  V(&nc->mutex);

  nc->prev = NULL;
  nc->next = cl->head;
  // nc->next->prev = nc;
  cl->head = nc;
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
  if (uribegin)
  {
    strcpy(uri, uribegin);
    hostend = uribegin;
  }
  else
  {
    uri[0] = '/';
    uri[1] = '\0';
    hostend = hostbegin + strlen(hostbegin);
  }

  // Look for port
  portbegin = strchr(hostbegin, ':');
  if (portbegin && portbegin < hostend)
  {
    hostlen = portbegin - hostbegin;
    strncpy(host, hostbegin, hostlen);
    host[hostlen] = '\0';
    portbegin++;
    int portlen = hostend - portbegin;
    strncpy(port, portbegin, portlen);
    port[portlen] = '\0';
  }
  else
  {
    hostlen = hostend - hostbegin;
    strncpy(host, hostbegin, hostlen);
    host[hostlen] = '\0';
    strcpy(port, "80");
  }
}
