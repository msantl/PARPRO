PROJECT = connect4
SOURCE = main.c list.c board.c
HEADERS = list.h board.h

CC = mpicc
CFLAGS = -Wall -g
LDFLAGS = -lncurses
OBJECTS = ${SOURCE:.c=.o}

$(PROJECT): $(OBJECTS)
	$(CC) $(CFLAGS) $(OBJECTS) -o $(PROJECT) $(LDFLAGS)

$(OBJECTS): $(HEADERS)

clean:
	-rm -f $(PROJECT) $(OBJECTS) *.core
