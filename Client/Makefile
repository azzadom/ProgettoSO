CC = gcc

CLIENT	= client

MAIN 	= main.c
FUNCTIONS = mainFunctions.c

all: $(CLIENT)

$(CLIENT): main.o func.o
	$(CC) -o $(CLIENT) main.o func.o

main.o: $(MAIN) globalVar.h
	$(CC) -o main.o $(MAIN) -c

func.o: $(FUNCTIONS) globalVar.h
	$(CC) -o func.o  $(FUNCTIONS) -c

clean:
	rm -f *.o
	rm -f $(CLIENT)