# Operating Systems Project

Thesis for the Operating Systems exam 22/23 with Professor Quaglia at the University of Rome Tor Vergata.

The project consists of a message exchange service supported by a concurrent server.

The service must accept messages from clients (generally hosted on different machines from the one where the server resides) and archive them. The client application must provide a user with the following functions:

1. Read all messages sent to the user
2. Send a new message to any user in the system
3. Delete messages received by the user

A message contains the fields: Recipient, Subject, and Text. Additionally, the service can only be used by authorized users through an authentication mechanism.

The operating system for which the service was written is LINUX.

For all implementation details, refer to the "relazione.pdf" file written in italian.

## Usage Instructions

### Server

The Makefile associated with the Server allows the creation of an object file from each source file, then compiles the executable "server" by combining the object files and libraries. Additionally, the following options are available:

- `make build`: Creates the directory containing the message files and indexes, and creates the authentication file.
- `make clean`: Deletes all object files and the executable.

To start the system for the first time:

1. Navigate to the "Server" folder in the terminal.
2. Run `make build` to set up the necessary directories and files.
3. Run `make` to compile the server executable.
4. Start the server with `./server`. To run it in the background, use `./server&`.

### Client

The Makefile associated with the Client allows the creation of an object file from each source file, then compiles the executable "client" by combining the object files and libraries. Additionally, the following option is available:

- `make clean`: Deletes all object files and the executable.

To run the client:

1. Before generating the client executable, open the file `Client/main.c` and modify `IPADDRESS` with the IPv4 address string of the machine where the server resides.
2. Navigate to the "Client" folder in the terminal.
3. Run `make` to compile the client executable.
4. Start the client with `./client` to test the system.

## Possible Improvements

- Improve mutex management so that only modifications to a file are blocking, not its reading.
- Enhance signal handling in the server to allow for a soft shutdown, for example through SIGTERM, which allows the program to close only after every active thread has completed its operations.
- Improve the acquisition of the subject and text of a message in the client by eliminating the management with "$END$" and using a signal like EOF instead.
