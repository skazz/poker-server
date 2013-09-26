CC=g++
CFLAGS=-I.
OBJ = dummyClient.o client.o pack.o
DEPS = client.h pack.h

%.o: %.cpp %.h
	$(CC) -c -o $@ $< $(CFLAGS)

client.o: client.cpp client.h pack.o
	$(CC) -c client.cpp

dummy: $(OBJ)
	$(CC) -o dummy dummyClient.o client.o pack.o $(CFLAGS)

server: server.o socketHandler.o pack.o seat.o deckC.o handEvaluator.o
	$(CC) -o server server.o pack.o seat.o deckC.o handEvaluator.o $(CFLAGS)

clean:
	rm *.o dummy server
