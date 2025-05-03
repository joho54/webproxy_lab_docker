/*
 * adder.c - a minimal CGI program that adds two numbers together
 */
/* $begin adder */
#include "csapp.h"

int main(void)
{
  INFO("running adder"); 
  
  char *buf, *p;
  char arg1[MAXLINE], arg2[MAXLINE], content[MAXLINE];
  int n1 = 0, n2 = 0;
  
  /* Extract the two arguments */
  if ((buf = getenv("QUERY_STRING")) != NULL)
  {
    
    INFO("unpacking env: %s", buf); 
    p = strchr(buf, '&');
    INFO("found '&' on %p", p); 
    *p = '\0';
    INFO("'&' replaced with whitespace. now buf: %s", buf); 
    strcpy(arg1, buf);
    INFO("arg1 strcpy done: %s", arg1); 
    strcpy(arg2, p + 1);
    INFO("arg2 strcpy done: %s", arg2); 
    n1 = atoi(strchr(arg1, '=') + 1);
    n2 = atoi(strchr(arg2, '=') + 1);
    INFO("unpacking completed"); 

  }

  INFO("start copying content");
  /* Make the response body */
  sprintf(content, "QUERY_STRING=%s\r\n<p>", buf);
  INFO("current content: %s", content);
  sprintf(content + strlen(content), "Welcome to add.com: ");
  INFO("current content: %s", content);
  sprintf(content + strlen(content), "THE Internet addition portal.\r\n<p>");
  INFO("current content: %s", content);
  sprintf(content + strlen(content), "The answer is: %d + %d = %d\r\n<p>",
  n1, n2, n1 + n2);
  INFO("current content: %s", content);
  sprintf(content + strlen(content), "Thanks for visiting!\r\n");
  INFO("current content: %s", content);

  /* Generate the HTTP response */
  printf("Content-type: text/html\r\n");
  printf("Content-length: %d\r\n", (int)strlen(content));
  printf("\r\n");
  printf("%s", content);
  fflush(stdout);

  exit(0);
}
/* $end adder */
