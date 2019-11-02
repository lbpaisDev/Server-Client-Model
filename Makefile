CC = gcc
OBJS = main.o 
PROG = Server

#####################

all:	${PROG}

clean:
	rm ${OBJS} *~ ${PROG} 

######################

main.o: ServerVF.c server.h
	${CC} ServerVF.c -c -o main.o

Server:	${OBJS}
	${CC} ${OBJS} -o Server -lcurl -ljson-c
