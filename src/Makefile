CC = gcc
PROG = run
OBJS = List.o terminal-chat.o
 
all: $(OBJS)
	#$(CC) -o $(PROG) $(OBJS)

List.o: List.c
	$(CC) -c -o List.o List.c

terminal-chat.o: terminal-chat.c
	$(CC) -Wall -g -o terminal-chat.o List.o terminal-chat.c -lpthread -lm

clean: 
	rm *.o