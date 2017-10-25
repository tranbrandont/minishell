/*    $Id: expand.c,v 1.59 2017/06/02 00:43:13 tranb7 Exp $    */

/* Brandon Tran
 * April 20, 2017
 * CS 352
 * Assignment 3
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <dirent.h>
#include <errno.h>



#include "proto.h"
#include "globals.h"


extern int argcount;
extern char** args;
extern int shift;
extern int exitstatus;
extern int sig;
extern int WAIT;
extern int DONTWAIT;
extern int infd;
extern int outfd;
extern int errfd;
int inQuotes;

//expand.c

int envreplace(char *readOrig, char *new, int *writeloc,
		 char *envstart, int newsize);
void pidreplace (char *readOrig, char *new, int *writeloc, int newsize);
void argreplace (char *readOrig, char *new, int *writeloc, int newsize);
void countargs(char *readOrig, char *new, int *writeloc, int newsize);
void replaceexit(char *readOrig, char *new, int *writeloc, int newsize);
void replacestar(char *readOrig, char *new, int *writeloc, int newsize);
void starSuffix(char *readOrig, char *new, int *writeloc, int newsize
		, int *foundfile);
int shellExec (char *readOrig, char *new, int *writeloc, int newsize);



//expands input string at certain points within the string
int expand(char *orig, char *new, int newsize) {
    char *readOrig = orig;
    int writeloc = 0;
    char *start = NULL;
	int foundfile = 0;
	int parens = 0;



    while (*readOrig != '\0') {
		if (sig == 1) {
			return -1;
		}
		if (writeloc > newsize) { //error if tries to write past array
	    	dprintf (errfd, "msh: expansion too long\n");
	    	return -1;
		}
		//If lone '#' without quotes, everything after is comment
		if(*readOrig == '"' && inQuotes == 0) {
			inQuotes = 1;
		}
		else if(*readOrig == '"' && inQuotes == 1) {
			inQuotes = 0;
		}
		if(inQuotes == 0 && *readOrig == '#') {
			while(*readOrig != '\0') {
				readOrig++;
			}
		}

	//writes * if preceded by \*
		if (*readOrig == '\\' && *(readOrig+1) == '*') {
			new[writeloc] = '*';
			(writeloc)++;
			readOrig += 2;
		}

		//prints all not starting with '.'
		if (*readOrig == '*' && ( *(readOrig-1) == ' ' ||
				 *(readOrig-1) == '"')) {
			if (*(readOrig+1) == ' ' ||
					*(readOrig+1) == '\0' ||
					*(readOrig+1) == '"') {
				replacestar(readOrig, new, &writeloc, newsize);
				readOrig++;
			}

			else { //prints everything ending in suffix
				starSuffix(readOrig, new, &writeloc,
					 newsize, &foundfile);
				if(foundfile) {
					while (*readOrig != ' ' &&
						*readOrig != '\0' && *readOrig != '"') {
						readOrig++;
					}
				}
			}
		}

		if (*readOrig == '$') {
			if (*(readOrig+1) == '{') { //${x}, get path of x
	    		start = readOrig+2;
				int envstat = envreplace (readOrig, new,
					 &writeloc, start, newsize);
	    		if (envstat == -1) {
		    		dprintf (errfd, "Odd number of braces\n");
		    		return -1;
	    		}
				else if (envstat == -2) {
		    		dprintf (errfd, "expansion too long\n");
	    	    	return -1;
				}
	    		while (*readOrig != '}') {
		    		readOrig++;
	    		}
	    		readOrig++;
	    	}
			else if (*(readOrig+1) == '(') { //$(, write to shell
				parens++;
				readOrig += 2;
				int shellStat = shellExec(readOrig, new, &writeloc, newsize);
				if (shellStat != 0) {
					return -1;
				}

				while(*readOrig != '\0') {

					if (*readOrig == '(') {
						parens++;
					}
					if (*readOrig == ')') {
						parens--;
						if (parens == 0) {
							break;
						}
					}
					readOrig++;
				}
				readOrig++;
			}
	    	else if (*(readOrig+1) == '$') { //$$, get PID
	    		pidreplace (readOrig, new, &writeloc, newsize);
	    		readOrig += 2;
	    	}
	    	else if(isdigit(*(readOrig+1)) != 0) { //$n, get arg n
	    		argreplace(readOrig, new, &writeloc, newsize);
    			while (isdigit(*(readOrig+1)) != 0) {
		    		readOrig++;
    			}
				readOrig++;
	    	}
			else if(*(readOrig+1) == '#') { //$#, get num of args
				countargs(readOrig, new, &writeloc, newsize);
				readOrig += 2;
			}

		 //$?, get exit value of previous command
			else if(*(readOrig+1) == '?') {
				replaceexit(readOrig, new, &writeloc, newsize);
				readOrig += 2;
			}
	    	else {
	    		new[writeloc] = *readOrig;
	    		(writeloc)++;
	    		readOrig++;
	    	}
		}

		else {
	    	new[writeloc] = *readOrig;
	    	(writeloc)++;
	    	readOrig++;
		}
    }
	new[writeloc] = '\0';
    return 0;

}

//replaces ${NAME} with the environment path
	int envreplace (char *readOrig, //the original array we're reading from
					char *new, //new array we're copying to
					int *writeloc, //location where we write to
					char *envstart, //first char of the environment
					int newsize) { //max size of new array
    char *env;

    while (*readOrig != '}' && *readOrig != '\0') {
		readOrig++;
    }


    if (*readOrig != '}') {
		return -1;
    }
    else {
		*readOrig = '\0';
    	env = getenv(envstart);


		if (env == NULL) {
		}
		else {
    		while (*env != '\0') {
				if (*writeloc > newsize) {
	    	    	return -2;
				}

	    		new[*writeloc] = *env;
	    		(*writeloc)++;
	    		env++;
    		}
		}
    *readOrig = '}';
    readOrig = readOrig;
    return 1;
    }
}

//replaces $$ with the pid of the shell
void pidreplace (char *readOrig, char *new, int *writeloc, int newsize) {
    int pid = getpid();
    char pidchar[newsize];
    int i = 0;
    int size = sizeof(pidchar);
    snprintf(pidchar, size, "%d", pid);
    while (pidchar[i] != '\0') {
		if (*writeloc > newsize) {
	    	dprintf (errfd, "expansion too long\n");
	    	return;
		}

		new[*writeloc] = pidchar[i];
		(*writeloc)++;
		i++;
    }
}

//Replaces $n with argument n
void argreplace (char *readOrig, char *new, int *writeloc, int newsize) {
    char* readstart = readOrig+1;
    char readend;
    char* arg = NULL;



    while (isdigit(*(readOrig+1)) != 0) {
		readOrig++;
    }

    readend = *(readOrig+1);
    *(readOrig+1) = '\0';

    int argnum = atoi(readstart)+1;

	if(argcount == 1 && argnum == 1) {
		arg = args[argnum-1];
	}

	else if(argnum == 1) {
		arg = args[argnum];
	}

    else if(argcount >= argnum + shift) {
    	arg = args[argnum + shift];
    }

    if(arg != NULL) {
    	while(*arg != '\0') {
	    	if (*writeloc > newsize) {
	    		dprintf (errfd, "expansion too long\n");
	    		return;
	    	}
	    	new[*writeloc] = *arg;
	    	(*writeloc)++;
	    	arg++;
    	}
    }
    *(readOrig+1) = readend;
}

//replaces $# with the number of arguments
void countargs(char *readOrig, char *new, int *writeloc, int newsize) {
    char argsize[newsize];
    int size = sizeof(argsize);
	int i = 0;


	if(argcount == 1) {
		snprintf(argsize, size, "%d", argcount);
	}
	else {
    	snprintf(argsize, size, "%d", (argcount - (shift + 1)));
	}

    while (argsize[i] != '\0') {

		if (*writeloc > newsize) {
	    	dprintf (errfd, "expansion too long\n");
	    	return;
		}

		new[*writeloc] = argsize[i];
		(*writeloc)++;
		i++;
    }
}

//replaces $? with the exit value of the previous command
void replaceexit(char *readOrig, char *new, int *writeloc, int newsize) {
	char exitsize[4];
	int size = sizeof(exitsize);
	int i = 0;

	snprintf(exitsize, size, "%d", exitstatus);
	while(exitsize[i] != '\0') {
		if (*writeloc > newsize) {
	    	dprintf (errfd, "expansion too long\n");
	    	return;
		}

		new[*writeloc] = exitsize[i];
		(*writeloc)++;
		i++;
	}
}

//wildcard expansion

void replacestar(char *readOrig, char *new, int *writeloc, int newsize) {
	char cwd[newsize];
	DIR *currdir;
	struct dirent *dp;
	char *filename;
	getcwd(cwd, newsize);

	if((currdir = opendir(cwd)) == NULL) {
		dprintf(errfd, "Unable to open directory: %s\n", strerror(errno));
		return;
	}
	while ((dp = readdir(currdir)) != NULL) { //if doesn't start with "."
		filename = dp->d_name;
		if(*filename != '.') {
			while (*filename != '\0') {
				if (*writeloc > newsize) {
		    		dprintf (errfd, "expansion too long\n");
		    		return;
				}
				new[*writeloc] = *filename;
				(*writeloc)++;
				filename++;
			}
			new[*writeloc] = ' ';
			(*writeloc)++;
		}
	}
	closedir(currdir);
}

//wildcard expansion with same suffix
void starSuffix(char *readOrig, char *new, int *writeloc, int newsize
		, int *foundfile) {

    char* readstart;
    char* readend;
	char readchar;

	char cwd[newsize];
	DIR *currdir;
	struct dirent *dp;
	char *filename;
	getcwd(cwd, newsize);
	int sufflength;
	int namelength;
	int wrote = 0;

	if((currdir = opendir(cwd)) == NULL) { //error can't open dir
		dprintf(errfd, "Unable to open directory: %s\n", strerror(errno));
		return;
	}

	//replaces end char with null
	readOrig++;
	readstart = readOrig;
    while (*readOrig != ' ' && *readOrig != '\0' && *readOrig != '"') {
		readOrig++;
    }
	readchar = *readOrig;
	readend = readOrig;
	*readOrig = '\0';
	readOrig = readstart;
	sufflength = strlen(readOrig);

	//reads through dir and prints files not starting with "."
	//and ending in suffix
	while((dp = readdir(currdir)) != NULL) {
		filename = dp->d_name;
		namelength = strlen(filename);
		if(sufflength <= namelength) {
			if(*filename != '.' &&
				strcmp(readOrig, &filename[namelength -
					 sufflength]) == 0) {

				*foundfile = 1;
				while (*filename != '\0') {
					if (*writeloc > newsize) {
		    			dprintf (errfd, "expansion too long\n");
		    			return;
					}
					new[*writeloc] = *filename;
					(*writeloc)++;
					filename++;
				}
				new[*writeloc] = ' ';
				(*writeloc)++;
				wrote = 1;
			}
		}
	}
	if(wrote == 1) {
		(*writeloc)--;
	}
	readOrig = readend;
	*readOrig = readchar;
	new[*writeloc] = *readOrig;
	closedir(currdir);
}

//outputs the input within parenthesis
int shellExec (char *readOrig, char *new, int *writeloc, int newsize) {
	int countParen = 1;
	int fd[2];
	char *readstart = readOrig;
	char buff[200000];
	int cpid;
	int status;
	int i = 0;



	while (countParen != 0 && *readOrig != '\0') {
		if (*readOrig == '(') {
			countParen++;
		}
		if (*readOrig == ')') {
			countParen--;
			if (countParen == 0) {
				*readOrig = '\0';
			}
		}
		readOrig++;
	}

	if (countParen != 0) {
		dprintf(errfd, "missing ) \n");
		return -1;
	}

	if (pipe(fd) < 0) {
		dprintf(errfd, "pipe: %s\n", strerror(errno));
		return -1;
	}
	cpid = processline(readstart, infd, fd[1], DONTWAIT | EXPAND);

	close(fd[1]);

	while (read(fd[0], buff, 1) != 0) {
		if (*writeloc > newsize) {
		    dprintf (errfd, "expansion too long\n");
		    return -1;;
		}
		new[*writeloc] = buff[0];
		(*writeloc)++;
	}
	new[*writeloc] = '\0';


	if (cpid > 0) {

		waitpid(cpid, &status, 0);
		if (WIFEXITED(status)) {
			exitstatus = WEXITSTATUS(status);
		}
		else if(WIFSIGNALED(status)) {
			exitstatus = 127;
		}
	}
	i = 0;
	while (i != *writeloc) {
		if (new[i] == '\n') {
			if (new[i+1] == '\0') {
				new[i] = '\0';
				(*writeloc)--;
				break;
			}
			else {
				new[i] = ' ';
			}
		}
		i++;
	}

	close(fd[0]);
	*(readOrig-1) = ')';
	return 0;
}
