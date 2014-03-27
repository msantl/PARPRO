PROJECT = filozofi
SOURCE = main.c list.c 
HEADERS = list.h

CC = mpicc
CFLAGS = -Wall -g -DNDEBUG
LDFLAGS =
OBJECTS = ${SOURCE:.c=.o}

$(PROJECT): $(OBJECTS)
	$(CC) $(OBJECTS) -o $(PROJECT)

$(OBJECTS): $(HEADERS)

clean:
	-rm -f $(PROJECT) $(OBJECTS) *.core
