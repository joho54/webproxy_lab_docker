/* $begin tinymain */
/*
 * tiny.c - A simple, iterative HTTP/1.0 Web server that uses the
 *     GET method to serve static and dynamic content.
 *
 * Updated 11/2019 droh
 *   - Fixed sprintf() aliasing issue in serve_static(), and clienterror().
 */
#include "csapp.h"

void doit(int fd);
void read_requesthdrs(rio_t *rp);
int parse_uri(char *uri, char *filename, char *cgiargs);
void serve_static(int fd, char *filename, int filesize);
void get_filetype(char *filename, char *filetype);
void serve_dynamic(int fd, char *filename, char *cgiargs);
void clienterror(int fd, char *cause, char *errnum, char *shortmsg,
                 char *longmsg);

int main(int argc, char **argv)
{
  int listenfd, connfd;
  char hostname[MAXLINE], port[MAXLINE];
  socklen_t clientlen;
  struct sockaddr_storage clientaddr;

  /* Check command line args */
  if (argc != 2)
  {
    fprintf(stderr, "usage: %s <port>\n", argv[0]);
    exit(1);
  }

  listenfd = Open_listenfd(argv[1]);
  while (1)
  {
    clientlen = sizeof(clientaddr);
    connfd = Accept(listenfd, (SA *)&clientaddr,
                    &clientlen); // line:netp:tiny:accept
    Getnameinfo((SA *)&clientaddr, clientlen, hostname, MAXLINE, port, MAXLINE, 0);
    printf("Accepted connection from (%s, %s)\n", hostname, port);
    doit(connfd);  // line:netp:tiny:doit
    Close(connfd); // line:netp:tiny:close
  }
}

/* 
Handles one HTTP transaction. 
*/
void doit(int fd) 
{
  int is_static;
  struct stat sbuf;
  char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
  char filename[MAXLINE], cgiargs[MAXLINE];
  rio_t rio;

  // First, we read and parse the request line.
  // Notice that we are using the rio_readlineb function to read the request line. ??여기서 왜 두 줄로 나눠 입력받는 건지 모르겠음.
  Rio_readinitb(&rio, fd);
  Rio_readlineb(&rio, buf, MAXLINE);
  printf("Request headers:\n");
  printf("%s", buf);
  sscanf(buf, "%s %s %s", method, uri, version); 

  // TINY supports only the GET method.
  // If the client requests another method, we send it an error message and return to the main routine, 
  // which then closes the connection and awaits the next connection request.
  // Otherwise, we read and (as we shall see) ignore any request headers.
  if (strcasecmp(method, "GET")){ 
    clienterror(fd, method, "501", "Not implemented", "Tiny does not implement this method");
    return;
  }
  read_requesthdrs(&rio);
  
  // Parse URI from GET request
  // Next, we parse the URI into a filename and a possibly empty CGI argument string and we set a flag that indicates whether the request is for static or dynamic content.
  is_static = parse_uri(uri, filename, cgiargs);
  // If the file does not exist on disk, we immediately send an error message to the client and return
  if (stat(filename, &sbuf) < 0)
  {
    clienterror(fd, filename, "404", "Not found", "Tiny couldn't find this file");
    return;
  }

  if (is_static)
  {
    // Finally, if the request is for static content, we verify that the file is a regular file and that we have read permission. 
    if (!(S_ISREG(sbuf.st_mode)) || !(S_IRUSR & sbuf.st_mode))
    {
      clienterror(fd, filename, "403", "Forbidden", "Tiny couldn't read the file");
      return;
    }
    // If so, we serve the static content to the client.
    serve_static(fd, filename, sbuf.st_size);
  }
  // Similarly, if the request is for dynamic content, we verify that the file is executable, and, if so, we go ahead and serve the dynamic content.
  else
  {
    if (!(S_ISREG(sbuf.st_mode)) || !(S_IXUSR & sbuf.st_mode))
    {
      clienterror(fd, filename, "403", "Forbidden", "Tiny couldn't run the CGI program");
      return;
    }
    serve_dynamic(fd, filename, cgiargs);
  }
}

/* 
clienterror function
TINY lacks many of the error-handling features of a real server.
However, it does check for some obvious errors and reports them to the client.
The clienterror function sends an HTTP response to the client with the appropriate status code and status message in the response line, 
along with an HTML file in the response body that explains the error to the browser's user.
Recall that an HTML response should indicate the size and type of the content in the body.
Thus, we have opted to build the HTML content as a single string so that we can easily determine its size. 
Also, notice that we are using the robust rio_writen function for all output.
*/
void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg)
{
  char buf[MAXLINE], body[MAXBUF];

  // build the HTTP response body
  sprintf(body, "<html><title>Tiny Error</title>");
  sprintf(body, "%s,body bgcolor=""ffffff"">\r\n", body);
  sprintf(body, "%s%s: %s\r\n", body, errnum, shortmsg);
  sprintf(body, "%s<p>%s: %s\r\n", body, longmsg, cause);
  sprintf(body, "%s<hr><em>The Tiny Web server</em>\r\n", body);

  sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg);
  Rio_writen(fd, buf, strlen(buf));
  sprintf(buf, "Content-type: text/html\r\n");
  Rio_writen(fd, buf, strlen(buf));
  sprintf(buf, "Content-length: %d\r\n\r\n", (int)strlen(body));
  Rio_writen(fd, buf, strlen(buf));
  Rio_writen(fd, body, strlen(body));
}


/* 
TINY does nto use any of the information in the request headers.
It simply reads and ignores them by calling the read_requesthdrs function.
Notice that the empty text line that termintaes the request headers consists of a carriage return and line feed pair, which we check for in line 6
*/
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

/* 
TINY assumes that the home directory for static content is its current directory and 
that the home directory for executables is ./cgi-bin.
The parse_uri function implements these policies. 
*/

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
    if (uri[strlen(uri)-1] == '/')  
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
      strcpy(cgiargs, ptr+1);
      *ptr = '\0';
    }
    else  
      strcpy(cgiargs, "");
    strcpy(filename, ".");
    strcat(filename, uri);
    return 0;
  }
}

/* 
TINY serves five common types of static content: HTML files, unformatted text files, and images encoded in GIF, PNG, and JPEG formats.
The serve_static function sends an HTTP response whose body contains the contents of a local file.
First, we determine the file type by inspecting the suffix in the filename and then send the response line and response headers to the client.
Notice that a blank line terminates the headers.
Next, we send the response body by copying the contents of the requested file to the connected descriptor fd.
The code here is somewhat subtle and needs to be studied carefully. 
*/

void serve_static(int fd, char *filename, int filesize)
{
  int srcfd;
  char *srcp, filetype[MAXLINE], buf[MAXBUF];
  
  // Send response headers to client
  get_filetype(filename, filetype);
  sprintf(buf, "HTTP/1.0 200 OK\r\n");
  sprintf(buf, "%sServer: Tiny Web Server\r\n", buf);
  sprintf(buf, "%sConnection: close\r\n", buf);
  sprintf(buf, "%sContent-length: %d\r\n", buf, filesize);
  sprintf(buf, "%sContent-type: %s\r\n\r\n", buf, filetype);
  Rio_writen(fd, buf, strlen(buf));
  printf("Response headers:\n");
  printf("%s", buf);

  //  Send response body to client
  // Line 18 opens filename for reading and gets its descriptor. 
  srcfd = Open(filename, O_RDONLY, 0);
  // In line 19, the Linux mmap function maps the requested file to a virtual memory area.
  // Recall from our discussion of mmap in Section 9.8 that the call to mmap maps the first filesize bytes of file srcfd 
  // to a private read-only area of virtual memory that starts at address srcp.
  srcp = Mmap(0, filesize, PROT_READ, MAP_PRIVATE, srcfd, 0);
  // Once we have mapped the file to memory, we no longer need its descriptor, so we close the file.
  // Failing to do this would introduce a potentially fatal memory leak.
  Close(srcfd);
  // Line 21 performs the actual transfer of the file to the client.
  // The rio_writen function copies the filesize bytes 
  // starting at location srcp (which of course is mapped to the requested file) to the client's connected descriptor.
  Rio_writen(fd, srcp, filesize);
  // Finally, line 22 frees the mapped virtual memory area. This is important to avoid a potentially fatal memory leak.
  Munmap(srcp, filesize);
}

void get_filetype(char *filename, char *filetype)
{
  if (strstr(filename, ".html"))
    strcpy(filetype, "text/html");
  else if (strstr(filename, ".gif"))
    strcpy(filetype, "image/gif");
  else if (strstr(filename, ".png"))
    strcpy(filetype, "image/png");
  else if (strstr(filename, ".jpg"))
    strcpy(filetype, "image/jpeg");
  else
    strcpy(filetype, "text/plain");
}

/* 
TINY serves any type of dynamic content by forking a child process and then running a CGI program in the context of the child.
The serve_dynamic function begins by sending a response line indicating success to the client, along with an informational Server header.
The CGI program is responsible for sending the rest of the response. 
Notice that this is not as robust as we might wish, since it doesn't allow for the possibility that the CGI program might encounter some error.
*/
void serve_dynamic(int fd, char *filename, char *cgiargs)
{
  char buf[MAXLINE], *emptylist[] = {NULL};

  // Return first part of HTTP response
  sprintf(buf, "HTTP/1.0 200 OK\r\n");
  Rio_writen(fd, buf, strlen(buf));
  sprintf(buf, "Server: Tiny Web Server\r\n");
  Rio_writen(fd, buf, strlen(buf));

  // After sending the first part of the response, we fork a new child process.
  if (Fork() == 0){
    // The child initializes the QUERY_STRING environment variable with the CGI arguments from the request URI.
    // Notice that a real server would set the other CGI environment variables here as well. For brevity, we have omitted this step.
    setenv("QUERY_STRING", cgiargs, 1);
    // Next, the child redirects the child's standard output to the connected file descriptor and then loads and runs the CGI program.
    // Since the CGI program runs in the context of the child, 
    // it has access to the same access to the same open files and environment variables that existed before the call to the execve function.
    // Thus, everything that the CGI program writes to standard output goes directly to the client process, 
    // without any intervention from the parent process.
    Dup2(fd, STDOUT_FILENO); // Redirect stdout to client
    Execve(filename, emptylist, environ); // Run CGI
  }
  // Meanwhile, the parent blocks in a call to wait, waiting to reap the child when it terminates.
  Wait(NULL); // Parent waits for and reaps child
}