/*    $Id: arg_parse.c,v 1.8 2017/05/31 06:04:10 tranb7 Exp $    */

/* Brandon Tran
 * April 6, 2017
 * CS 352
 * Assignment 2
 */



#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include "proto.h"





char ** arg_parse(char *line, int *argcp) {
    char* original = line; //keeps starting point of line
    char **line_array = NULL;
    int skipPointer = 0; //adjusts the pointer for quotes

    //counts number of arguments
    while (*line != '\0') {
        if (*line != ' ') {
	    	(*argcp)++;
            while (*line != ' ' && *line != '\0') {
                if (*line == '"') {
		    		line++;
		    		while (*line != '"' && *line != '\0') {
		        		line++;
		    		}
		    		if (*line != '"') {
						fprintf (stderr
							, "Odd number of quotes\n");
						*argcp = 0;
						return line_array;
		    		}
	        	}
                line++;
            }

	    	if (*line != '\0') {
                *line = '\0';
	        	line++;
	    	}
        }
		else if (*line == ' ') {
	    	line++;
		}
    }


 //allocates space for pointers equal to number of arguments and null pointer
    line_array = (char **)malloc((*argcp + 1) * sizeof(char *));
    
    //checks if malloc was successful
    if(line_array == NULL) {
		perror ("malloc");
		return 0;
    }

    //sets pointers to first char of each argument, and adjusts pointer for
    //future '"' removal
    line = original;

    for(int i = 0; i < *argcp; i++) {
        while(*line == ' ') {
            line++;
        }
        line_array[i] = line - skipPointer;
        while(*line != ' ' && *line != '\0') {
	    	if (*line == '"') {
				skipPointer += 2;
				line++;
				while (*line != '"' && *line != '\0') {
		        	line++;
				}
	    	}
            line++;
        }
	line++;
    }

    line_array[*argcp] = '\0'; //sets last pointer to null

    //Removes quotes from arguments
    line = original;
    rmvquotes(line, argcp);

    return line_array;
}

//removes quotes from arguments
void rmvquotes(char *line, int *argcp) {
    char *read = line;
    char *write = line;

    for (int i = 0; i < *argcp;) {
		*write = *read++;
		write += (*write != '"');
        i += (*read == '\0');
    }
	if(*write != '\0') {
    	*write = '\0';
	}
}


