
#include <stdio.h>
#include <sys/types.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>
#include <time.h>

#include <signal.h>


// Instead of passing arguments, these objects can be static to be shared by following methods
int socketEndpoint;
struct sockaddr_in socketAddress;

// openConnection can be a client connection by Server, or the socketEndpoint by Client
int openConnection;


// For simpler structure, different running statuses are separated into methods
int runAsServer(void);
int runAsClient(char *hostingAddress);

// Since loops for writing and reading are same for Server & Client, they are encapsulated as methods
void writeToConnection(void);
void readFromConnection(void);


int main(int argc, const char * argv[]) {
	printf("\nSOCKET MESSENGER\n");

	// Initialize the socket. If initialization fails, end the process
	// Socket is configured to use IPv4 Internet protocols, Two-way TCP stream
	socketEndpoint = socket(AF_INET, SOCK_STREAM, 0);

	if (socketEndpoint < 0) {
		printf("\n Error : Could not create socket \n");
		return -1;
	}


	// Initialize the address struct.
	// Using hton(), change byte-order for any hosting address and port number 5000
	bzero(&socketAddress, sizeof(socketAddress));
	socketAddress.sin_family = AF_INET;
	socketAddress.sin_addr.s_addr = htonl(INADDR_ANY);
	socketAddress.sin_port = htons(5000);


	// This one program can run as Server or Client
	// At launch, the process evaluates arguments to see if it should run as server.
	if (argc > 1 && strcmp(argv[1], "server") == 0) {
		openConnection = runAsServer();
	}
	else {
		// The program assume that usual localhostIP is 127.0.0.1, or the user can change it by using arv[1]
		char *hostingAddress = (argc > 1) ? (char*)argv[1]:"127.0.0.1";
		openConnection = runAsClient(hostingAddress);
	}

	if (openConnection < 0) {
		return -1;
	}


	// getline() is needed for both Server and Client, but since it's a blocking method,
	// extra background process should run to read openConnection.
	// By using a 2 processes, reading and writing (by user) can operate concurrently from Server and Client

	pid_t pid = fork();

	// Reading is executed within the background process
	if (pid == 0) {
		readFromConnection();
	}
	else {
		writeToConnection();
	}


	// At this point, while-loop is broken, ready to terminate the process
	close(openConnection);

	// Kill the background process
	if (pid == 0) {
		printf("kill: %d\n", pid);
		kill(pid, SIGTERM);
	}

	return 0;
}


int runAsServer() {
	printf("RUN AS SERVER...\n");

	// Open socketEndpoint with assigned address and port number.
	// Wait for a client to connect by listening to socketEndpoint
	if (bind(socketEndpoint, (struct sockaddr*)&socketAddress, sizeof(socketAddress)) < 0) {
		printf("\n Error : Bind Failed \n");
		return -1;
	}


	// Listen to connection request with waiting queue of 1
	listen(socketEndpoint, 1);

	printf("\nLISTENING... \n");


	// When a client is connected, an endpoint is returned.
	int clientConnection = accept(socketEndpoint, (struct sockaddr*)NULL, NULL);

	if (clientConnection > 0) {
		time_t timestamp = time(NULL);
		printf("CLIENT %d CONNECTED AT: %.24s\n", clientConnection, ctime(&timestamp));
	}

	// Returned clientConnection(== openConnection) will be used for reading and writing.

	return clientConnection;
}

int runAsClient(char *hostingAddress) {
	printf("RUN AS CLIENT...\n");

	// Convert text address to integer binary form
	if (inet_pton(AF_INET, hostingAddress, &socketAddress.sin_addr) <= 0) {
		printf("\nhostingAddress: %s\n", hostingAddress);
		printf("\n inet_pton error occured\n");

		return -1;
	}


	// Client attempts connecting to hostingAddress. If connecting fails, end the process
	if (connect(socketEndpoint, (struct sockaddr *)&socketAddress, sizeof(socketAddress)) < 0) {
		printf("\n Error : Connect Failed \n");
		printf("\nTo run as server, type: \"./socketMessenger.out server\"\n");

		return -1;
	}


	time_t timestamp = time(NULL);
	printf("CONNECTED TO SERVER %s AT: %.24s\n", hostingAddress, ctime(&timestamp));


	// Returned socketEndpoint(== openConnection) will be used for reading and writing.

	return socketEndpoint;
}


void writeToConnection(void) {

	printf("\nSTART ENTERING MESSAGES\n(Type \"CLOSE\" to end connection):\n\n");

	// Repeat waiting for user to enter a line, sending it to connection
	while(1) {
		fputs("THIS USER: ", stdout);	// A prompt
		
		char *message = NULL;
		size_t bufferSize = 0;
		ssize_t inputSize = getline(&message, &bufferSize, stdin);

		// Skip, in case of just hitting return \n
		if (inputSize <  1 || strcmp(message, "\n") == 0) {
			continue;
		}


		// Write strlen of characters to openConnection
		write(openConnection, message, strlen(message));


		// Use "CLOSE" as a command, breaking the loop and return back to main()
		if (strstr(message, "CLOSE")) {
			break;
		}
	}
}

void readFromConnection(void) {

	while (1) {
		// Try reading set length from connection, though it may include trailing zeros
		char message[1024];
		bzero(message, sizeof(message));

		ssize_t messageSize = read(openConnection, message, sizeof(message)-1);

		if (messageSize < 0) {
			fputs("\n Read error \n", stdout);
			break;
		}


		// Disconnect trailing zeros by nullifying the end of the string
		message[messageSize] = 0;


		// Use "CLOSE" as a command, breaking the loop and return back to main()
		// If trimmed string length is actually 0, also break the loop
		if (strstr(message, "CLOSE") || strlen(message) <= 0) {
			break;
		}


		// To distinguish the received message from local messages,
		// a identification prompt is added in front of the string.
		char formatted[1024];
		bzero(formatted, sizeof(formatted));
		snprintf(formatted, sizeof(formatted), "\nFROM connection %d: %s", openConnection, message);

		// Print formatted message to standard output
		if (fputs(formatted, stdout) == EOF) {
			printf("\n Error : Fputs error\n");
		}

		// Because receiving end was also running as writer,
		// the prompt should be restored after printing received message.
		fputs("THIS USER: ", stdout);
		fflush(stdout);
	}
}
