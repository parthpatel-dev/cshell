OBJS	= cshell
SOURCE	= cshell.c
OUT	= cshell
CC	 = gcc
FLAGS	 = -g -Wall

all: cshell

cshell: cshell.c
	$(CC) $(FLAGS) -o $(OBJS) cshell.c 

clean:
	rm -f $(OBJS) $(OUT)