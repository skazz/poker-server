CC = g++
DEBUG = -g
CFLAGS = -Wall -c $(DEBUG)
LFLAGS = -Wall $(DEBUG)
OBJ = server.o pack.o log.o seat.o deckC.o handEvaluator.o
DEPS = $(OBJ) socketHandler.o


server: $(DEPS)
	$(CC) $(LFLAGS) $(OBJ) -o server

dummy: dummyClient.o client.o pack.o
	$(CC) dummyClient.o client.o pack.o -o dummy

%.o: %.cpp %.h
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm *.o dummy server

tar:
	tar czvf poker.tar.gz *.cpp *.h makefile README TODO list_of_actions handranks
