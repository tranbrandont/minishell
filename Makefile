#    $Id: Makefile,v 1.5 2017/05/31 06:04:10 tranb7 Exp $
# Makefile for msh



CC=gcc
OBJ = msh.o builtin.o arg_parse.o expand.o strmode.o redirection.o
CFLAGS = -g -Wall
.SUFFIXES: .c .o

.c.o:; $(CC) $(CFLAGS) -c $< 


msh: $(OBJ)
	gcc $(CFLAGS) -o $@ $^

clean:
	rm -rf $(FILES) $(OBJ) msh
