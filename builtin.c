/*    $Id: builtin.c,v 1.34 2017/06/01 23:11:26 tranb7 Exp $    */

/* Brandon Tran
 * April 9, 2017
 * CS 352
 * Assignment 2
 */



#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <errno.h>



#include "proto.h"
#include "globals.h"



int builtin(char **line, int *argcp);
void bi_exit(char **line, int *argcp);
int bi_aecho(char **line, int *argcp);
int bi_envset(char **line, int *argcp);
int bi_envunset(char **line, int *argcp);
int bi_cd(char **line, int *argcp);
int bi_shift(char **line, int *argcp);
int bi_unshift(char **line, int *argcp);
int bi_sstat(char **line, int *argcp);
int bi_read(char **line, int *argcp);

int output = 1;
int infd = 0;
int errfd = 2;

extern int shift;
extern int argcount;
extern int errfd;

/* Constants */

#define LINELEN 1024

//jumps to function that matches builtin command
int runbuiltin(char **line, int *argcp, int command, int outfd, int in, int err) {

	output = outfd;
	infd = in;
	errfd = err;

	int success;

    switch(command) {
        case 0 :
	    	bi_exit(line, argcp);
	    	break;

		case 1 :
	    	success = bi_aecho(line, argcp);
	    	break;

		case 2 :
	    	success = bi_envset(line, argcp);
	    	break;

		case 3 :
		    success = bi_envunset(line, argcp);
		    break;

		case 4 :
		    success = bi_cd(line, argcp);
			break;

		case 5 :
			success = bi_shift(line, argcp);
			break;

		case 6 :
			success = bi_unshift(line, argcp);
			break;

		case 7 :
			success = bi_sstat(line, argcp);
			break;

		case 8 :
			success = bi_read(line, argcp);

    }
	return success;
}

//checks if command is builtin
int builtin(char **line, int *argcp) {
    const char *flist[] = {"exit", "aecho", "envset", "envunset", "cd"
		, "shift", "unshift", "sstat", "read"};
    int functionCount = sizeof(flist) / sizeof(flist[0]);

    for(int i = 0; i < functionCount; i++) {
		if(strcmp(line[0], flist[i]) == 0) {
	    	return i;
		}
    }
    return -1;
}

//exits out of program with given value, 0 if value not given
void bi_exit(char **line, int *argcp) {

    int exit_val;

    if (*argcp < 2) {
		exit_val = 0;
    }

    else {
        exit_val = atoi(line[1]);
    }

    exit(exit_val);
}

//echoes arguments
int bi_aecho(char **line, int *argcp) {

    if (*argcp < 2) {
		dprintf(output, "\n");
		return 0;
    }
    else if (strcmp(line[1], "-n") == 0) {
		if (*argcp < 3) {
		}

		else {
	    	dprintf (output, "%s", line[2]);
	    	for (int i = 3; i < *argcp; i++) {
	    		dprintf (output, " %s", line[i]);
	    	}
		}
		return 0;
    }

    else {
		dprintf (output, "%s", line[1]);
		for (int i = 2; i <= *argcp; i++) {
	    	if (line[i] == '\0') {
				dprintf (output, "\n");
	    	}
	    	else {
	    		dprintf (output, " %s", line[i]);
	    	}
 		}
		return 0;
    }
}

//changes the environment to the given value
int bi_envset(char **line, int *argcp) {
    if (*argcp != 3) {
		dprintf (errfd, "usage: envset name value\n");
		return 1;
    }
    else {
		int error = setenv(line[1], line[2], 1);
		if (error == -1) {
	    	dprintf(errfd, "envset: %s\n", strerror(errno));
			return 1;
		}
		return 0;
    }
}

//deletes the given environment name
int bi_envunset(char **line, int *argcp) {
    if (*argcp != 2) {
		dprintf (errfd, "usage: envunset name\n");
		return 1;
    }

    else {
		int error = unsetenv(line[1]);
		if (error == -1) {
	    	dprintf(errfd, "unsetenv: %s\n", strerror(errno));
			return 1;
		}
		return 0;
    }
}

//changes working directory to given directory
int bi_cd(char **line, int *argcp) {
    if (*argcp > 2) {
		dprintf (errfd, "usage: cd [dir]\n");
		return 1;
    }

    else if (*argcp == 2) {
		if (chdir(line[1]) == -1) {
		    dprintf(errfd, "chdir: %s\n", strerror(errno));
			return 1;
		}
		return 0;
    }

    else {
		if (getenv("HOME") == NULL) {
	    	dprintf (errfd, "cd: Environment variable HOME not set\n");
			return 1;
		}
		else {
		    chdir(getenv("HOME"));
			return 0;
		}
    }
}

//shifts all arguments by n values starting at arg 1
int bi_shift(char **line, int *argcp) {
	if (*argcp == 1) {
		shift += 1;
		return 0;
	}

	else if (*argcp == 2) {
		int shiftamt = atoi(line[1]);

		if (shiftamt+shift > argcount) {
			dprintf(errfd, "shift: not enough parameters for shift of %d\n",
				shiftamt);
			return 1;
		}
		else {
			shift += shiftamt;
			return 0;
		}
	}
	else {
		dprintf(errfd, "usage: shift [n]\n");
		return 1;
	}
}


//unshifts all arguments by n values or unshifts completely
int bi_unshift(char **line, int *argcp) {
	if (*argcp == 1) {
		shift = 0;
		return 0;
	}

	else if(*argcp == 2) {
		int shiftamt = atoi(line[1]);
		if (shiftamt >= shift) {
			dprintf(errfd, "unshift: not enough parameters for shift of %d\n",
				shiftamt);
			return 1;
		}
		else {
			shift -= shiftamt;
			return 0;
		}
	}

	else {
		dprintf(errfd, "usage: unshift [n] \n");
		return 1;
	}
}

int bi_sstat(char **line, int *argcp) {
	struct stat filestat;
	struct passwd *pw;
	struct group *grp;
	char *username;
	char *groupname;
	char mode[10];


	//no file provided
	if (*argcp == 1) {
		dprintf(errfd, "No file entered\n");
		return 1;
	}


	else {
		int i = 1;
		while(line[i] != NULL) {

			//if stat can't get stat for file
			if(stat(line[i], &filestat) == -1) {
				dprintf(errfd, "stat: %s\n", strerror(errno));
				return 1;
			}

			else {
				uid_t uid = (&filestat)->st_uid;
				gid_t gid = (&filestat)->st_gid;
				mode_t filemode = (&filestat)->st_mode;
				nlink_t links = (&filestat)->st_nlink;
				off_t filesize = (&filestat)->st_size;

				pw = getpwuid(uid);
				grp = getgrgid(gid);


				dprintf(output, "%s ", line[i]);

				if (pw && (username = pw->pw_name) != NULL) {
					dprintf(output, "%s ", username);
				}

				else {
					dprintf(output, "%d ", uid);
				}

				if (grp && grp->gr_name != NULL) {
					groupname = grp->gr_name;
					dprintf(output, "%s ", groupname);
				}

				else {
					dprintf(output, "%d ", gid);
				}
				strmode(filemode, mode);
				dprintf(output, "%s%lu %ld %s", mode, links
					, filesize, ctime	 (&filestat.st_mtime));

			}
			i++;
		}
	}
	return 0;

}

//takes the name of an env variable, and defines the env variable as the
//following line from stdin
int bi_read(char **line, int *argcp) {
	char userInput[LINELEN];

	int i = 0;

	if (*argcp != 2) {
		dprintf(errfd, "Usage : read variable-name\n");
		return 1;
	}
	else {
		read(infd, userInput, LINELEN);
		while (userInput[i] != '\n') {
			i++;
		}
		userInput[i] = '\0';
		int error = setenv (line[1], userInput, 1);
		if (error == -1) {
	    	dprintf(errfd, "envset: %s\n", strerror(errno));
			return 1;
		}
	}
	return 0;

}
