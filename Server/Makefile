CC = gcc

SERVER	= server

MAIN = main.c
FUNCTIONS = mainFunctions.c
AUTH = authFunctions.c
MSG = messageFunctions.c

DIRMSG = FileMessaggi
AUTHFILE = credenziali.txt

all: $(SERVER)

$(SERVER): main.o func.o auth.o message.o
	$(CC) -o $(SERVER) main.o func.o auth.o message.o -lcrypt -lpthread

main.o: $(MAIN) mainFunctions.h globalVar.h
	$(CC) -o main.o $(MAIN) -c -lpthread -lcrypt

func.o: $(FUNCTIONS) mainFunctions.h globalVar.h authFunctions.h messageFunctions.h
	$(CC) -o func.o  $(FUNCTIONS) -c -lpthread -lcrypt

auth.o: $(AUTH) authFunctions.h
	$(CC) -o auth.o $(AUTH) -c

message.o: $(MSG) messageFunctions.h
	$(CC) -o message.o $(MSG) -c -lpthread

build: 
	@mkdir $(DIRMSG)
	@touch $(AUTHFILE)

clean:
	rm -f *.o
	rm -f $(SERVER)