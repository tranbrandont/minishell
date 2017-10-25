/* $Id: redirection.c,v 1.5 2017/06/01 23:11:26 tranb7 Exp $ */

/* Brandon Tran
 * May 28, 2017
 * CS 352
 * Assignment 6
 */


 #include <stdio.h>
 #include <stdlib.h>
 #include <sys/types.h>
 #include <sys/stat.h>
 #include <string.h>
 #include <fcntl.h>
 #include <unistd.h>
 #include <errno.h>
 #include "proto.h"
 #include "globals.h"


char findEndFile (char *line, int inquotes);
void removeQuote (char *line);


/*Globals */


int redirection (char *line, int *infd, int *outfd, int *errfd) {
	int inquotes = 0;
  char afterFile;

  while (*line != '\0') {
    if (*line == '\"') {
      inquotes = !inquotes;
      line++;
    }
  	if (*line == '<' && !inquotes) {
      afterFile = findEndFile(line, inquotes);
      while (*line == ' ') {
          line++;
        }
      int fd = open(line, O_RDONLY, 0666);

      if(fd == -1) {
        dprintf(*errfd, "open: %s\n", strerror(errno));
        return 1;
      }
      if (*infd != 0) {
        close(*infd);
      }
      *infd = fd;
      while (*line != '\0') {
        *line = ' ';
        line++;
      }
      *line = afterFile;
    }
    else if (*line == '>' && !inquotes) {
      *line = ' ';
      line++;
      if(*line == '>') {
        afterFile = findEndFile(line, inquotes);
        while (*line == ' ') {
          line++;
        }

        int fd = open(line, O_RDWR | O_APPEND | O_CREAT, 0666);
        if(fd == -1) {
          dprintf(*errfd, "open: %s\n", strerror(errno));
          return 1;
        }
        if (*outfd != 1) {
          close(*outfd);
        }
        *outfd = fd;
        while (*line != '\0') {
          *line = ' ';
          line++;
        }
        *line = afterFile;
      }
      else {
        line--;
        afterFile = findEndFile(line, inquotes);
        while (*line == ' ') {
          line++;
        }
        int fd = open(line, O_RDWR | O_CREAT | O_TRUNC, 0666);
        if(fd == -1) {
          dprintf(*errfd, "open: %s\n", strerror(errno));
          return 1;
        }
        if (*outfd != 1) {
          close(*outfd);
        }
        *outfd = fd;
        while (*line != '\0') {
          *line = ' ';
          line++;
        }
        *line = afterFile;
      }
    }
    else if (*line == '2' && (*(line-1) == ' ' || *(line-1) == '\0')
              && *(line+1) == '>') {
      *line = ' ';
      line += 2;
      if(*line == '>') {
        *(line-1) = ' ';
        afterFile = findEndFile(line, inquotes);
        while (*line == ' ') {
          line++;
        }
        int fd = open(line, O_RDWR | O_APPEND | O_CREAT, 0666);
        if(fd == -1) {
          dprintf(*errfd, "open: %s\n", strerror(errno));
          return 1;
        }
        if (*errfd != 2) {
          close(*errfd);
        }
        *errfd = fd;
        while (*line != '\0') {
          *line = ' ';
          line++;
        }
        *line = afterFile;
      }
      else {
        line--;
        afterFile = findEndFile(line, inquotes);
        while (*line == ' ') {
          line++;
        }
        int fd = open(line, O_RDWR | O_CREAT | O_TRUNC, 0666);
        if(fd == -1) {
          dprintf(*errfd, "open: %s\n", strerror(errno));
          return 1;
        }
        if (*errfd != 2) {
          close(*errfd);
        }
        *errfd = fd;
        while (*line != '\0') {
          *line = ' ';
          line++;
        }
        *line = afterFile;
      }
    }
    line++;
  }
  return 0;
}

char findEndFile (char *line, int inquotes) {
  char* filename;

  *line = ' ';
  line++;
  while (*line == ' ') {
    line++;
  }
  filename = line;
  while (*filename != ' ' && *filename != '>' && *filename != '<' &&
          *filename != '\0') {
    if(*filename == '\"' && !inquotes) {
	  filename++;
      while (*filename != '\"') {
		filename++;
	  }
    }
	filename++;
  }
  char lastChar = *filename;
  *filename = '\0';
  removeQuote(line);
  return lastChar;
}


void removeQuote (char *line) {
  char *read = line;
  char *write = line;

  while (*read != '\0') {
	*write = *read++;
	write += (*write != '"');
  }
  if(*write != '\0') {
    *write = '\0';
  }
}
