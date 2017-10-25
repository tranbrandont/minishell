/*    $Id: msh.c,v 1.48 2017/06/02 03:38:15 tranb7 Exp $    */

/* CS 352 -- Mini Shell!
 *
 *   Sept 21, 2000,  Phil Nelson
 *   Modified April 8, 2001
 *   Modified January 6, 2003
 *
 */


/* Brandon Tran
 * March 29, 2017
 * CS 352
 * Assignment 2
 */

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "proto.h"
#include "globals.h"





/* Constants */

#define LINELEN 200000

/* Globals */

extern int argcount;
extern char** args;
extern int shift;
extern int exitstatus;
extern int sig;
extern int WAIT;
extern int DONTWAIT;
extern int EXPAND;
extern int DONTEXPAND;
FILE* input;


/* Prototypes */


void sig_handler (int signo, siginfo_t *siginfo, void *context);
int findunquote (char *buf, char c);
int pipeline (char* line, int cinfd, int coutfd, int cerrfd);

/* Shell main */


int
main (int argc, char **argv)
{
    char   buffer [LINELEN];
    int    len;
    argcount = argc;
    args = argv;
	struct sigaction act;
	WAIT = 1;
	DONTWAIT = 0;
	EXPAND = 4;
	DONTEXPAND = 0;





	act.sa_sigaction = &sig_handler;
	act.sa_flags = SA_SIGINFO + SA_RESTART;


	if (sigaction(SIGINT, &act, NULL) < 0) {
		dprintf(2, "sigaction: %s\n", strerror(errno));
		return 1;
	}

    if (argc > 1) {
		input = fopen(argv[1], "r");
		if(input == NULL) {
		    dprintf(2, "Could not open file.\n");
		    exit(127);
		}
    }
    else {
		input = stdin;
    }

    while (1) {
		char* p1 = getenv("P1");

		memset (&act, '\0', sizeof(act));


    	/* prompt and get line */
		if(argc == 1) {
			if (p1 != NULL) {
				fprintf (stderr, "%s ", p1);
			}
			else {
	    		fprintf (stderr, "%% ");
			}
		}
		if (fgets (buffer, LINELEN, input) != buffer)
	    	break;

    	/* Get rid of \n at end of buffer. */
		len = strlen(buffer);
		if (buffer[len-1] == '\n')
	    	buffer[len-1] = 0;

		/* Run it ... */
		sig = 0;


		processline (buffer, 0, 1, WAIT | EXPAND);
		

    }


    if (!feof(stdin) && argc == 1)
        dprintf(2, "read: %s\n", strerror(errno));

    return 0;		/* Also known as exit (0); */
}


int processline (char *line, int infd, int outfd, int flags)
{
	char newarray[LINELEN];
    pid_t  cpid;
    int    status;
	int argcp = 0;
	int cinfd = infd;
	int coutfd = outfd;
	int cerrfd = 2;

	while (waitpid(-1, &status, WNOHANG) > 0);

	if(flags & EXPAND) {
    	if (expand(line, newarray, LINELEN) == -1) {
			return -1;
    	}
	}
	else {
		strcpy(newarray, line);
	}

	if (flags & EXPAND) {
		if (pipeline(newarray, cinfd, coutfd, cerrfd) == 1) {
			return 0;
		}
	}

	if(redirection(newarray, &cinfd, &coutfd, &cerrfd) == 1) {
		return -1;
	}




    char **line_parsed = arg_parse(newarray, &argcp);
    if (argcp == 0) {
		return -1;
    }
    //checks for builtin commands
    int checkbuiltin = builtin(line_parsed, &argcp);


    /* Start a new process to do the job. */
    if (checkbuiltin == -1) {
    	cpid = fork();
        if (cpid < 0) {
            if (line_parsed) {
	        	free (line_parsed);
	    	}
            dprintf(cerrfd, "fork: %s\n", strerror(errno));
            return -1;
    	}
    	/* Check for who we are! */
    	if (cpid == 0) {
            /* We are the child! */
			if (sig == 1) {
				raise(SIGINT);
			}
			if (cinfd != 0) {
				dup2(cinfd, 0);
			}
			if (coutfd != 1) {
				dup2(coutfd, 1);
			}
			if (cerrfd != 2) {
				dup2(cerrfd, 2);
			}

            execvp (line_parsed[0], line_parsed);
            dprintf(cerrfd, "exec: %s\n", strerror(errno));
			fclose(input);
            exit (127);
    	}

    	//frees malloced space
    	if (line_parsed) {
	    	free (line_parsed);
    	}

    	/* Have the parent wait for child to complete */
		if (flags & WAIT) {
			if (waitpid(cpid, &status, 0) < 0) {
            	dprintf(cerrfd, "wait: %s\n", strerror(errno));
			}
			else {
				if (WIFEXITED(status)) {
					exitstatus = WEXITSTATUS(status);
				}
				else if(WIFSIGNALED(status)) {
					exitstatus = 127;
				}
			}
		}
    }

    else {
		exitstatus = runbuiltin(line_parsed, &argcp, checkbuiltin, coutfd, 
								cinfd, cerrfd);
        if (line_parsed) {
	    	free (line_parsed);
		}
    }
	if (cinfd != infd) {
		close(cinfd);
	}
	if (coutfd != outfd) {
		close(coutfd);
	}
	if (cerrfd != 2) {
		close(cerrfd);
	}
	return cpid;
}
//handles sigint
void sig_handler (int signo, siginfo_t *siginfo, void *context) {
	sig = 1;
}


int pipeline (char* line, int cinfd, int coutfd, int cerrfd) {
	int inquotes = 0;
	int pipeCount = 0;
	int fd[2];
	char* lineStart = line;
	int pipePresent = 0;
	
	while (*line != '\0') {
    	if (*line == '\"') {
        	inquotes = !inquotes;
        	line++;
    	}	
		if (*line == '|' && !inquotes) {
			pipeCount++;
			*line = '\0';
			pipePresent = 1;
		}

		line++;
	}
	if (pipePresent == 1) {
		char *pipeStarts[pipeCount+1];
		line = lineStart;
		pipeStarts[0] = lineStart;
		int i = 0;
		while (*line != '\0' || pipeCount != i) {
			if (*line == '\0') {
				line++;
				i++;
				pipeStarts[i] = line;
			}		
			line++;
		
		}
		int infd = cinfd;
		for (int i = 0; i < pipeCount; i++) {
			if (pipe(fd) < 0) {
				dprintf(cerrfd, "pipe: %s\n", strerror(errno));
				return -1;
			}
			processline (pipeStarts[i], infd, fd[1], DONTWAIT | DONTEXPAND);
			close(fd[1]);			
			if (infd != cinfd) {
				close(infd);
			}
			infd = fd[0];
		}
		processline (pipeStarts[i], infd, coutfd, WAIT | DONTEXPAND);
		close(infd);
	}

	return pipePresent;
}
