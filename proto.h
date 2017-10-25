/*    $Id: proto.h,v 1.19 2017/06/01 23:11:26 tranb7 Exp $    */

/* Brandon Tran
 * April 6, 2017
 * CS 352
 * Assignment 2
 */

int processline (char *line, int infd, int outfd, int flags);
char ** arg_parse(char *line, int *argcp);
void rmvquotes(char *line, int *argcp);
int builtin(char **line, int *argcp);
int runbuiltin(char **line, int *argcp, int command, int outfd, int in, int err);

int expand(char *orig, char *new, int newsize);
int redirection (char *buf, int *infd, int *outfd, int *errfd);


//strmode.c
void strmode(mode_t mode, char *p);
