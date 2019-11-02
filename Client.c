#include <stdio.h>	
#include <stdlib.h> 
#include <string.h> 
#include <sys/socket.h>	
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>	
#include <time.h>
#include <signal.h>
#include <fcntl.h>
#include <syslog.h>
#include <sys/wait.h>
#include <sys/file.h>

#define BUF_SIZE 1024

void error(char*msg);
void send_response(int sock, char response[BUF_SIZE]);
void request_info(int sock);

int main(int argc, char *argv[]){
	char serverAddr[100];
	int sock;
	struct sockaddr_in c_addr;
	struct hostent *hostPtr;

	if(argc != 3){
		printf("./Client <host> <port> \n");
		exit(-1);
	}

	strcpy(serverAddr, argv[1]);
	if((hostPtr = gethostbyname(serverAddr)) == 0){
		error("Couldn't get address");
	}

	bzero((void *) &c_addr, sizeof(c_addr));
    c_addr.sin_family = AF_INET;
    c_addr.sin_addr.s_addr = ((struct in_addr *)(hostPtr->h_addr))->s_addr;
    c_addr.sin_port = htons((short) atoi(argv[2]));

    if((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0){
    	error("Couldn't create socket");
    }

    if((connect(sock, (struct sockaddr *)&c_addr, sizeof c_addr)) < 0){
    	error("Couldn't connect to the server");
    }
	  	
	request_info(sock);
	
	close(sock);
	exit(0);

	return 0;
}

//Error detection
void error(char *msg){
	printf("-----Fatal Error: %s -----\n",msg);
	exit(-1);
}
 
//Send server response
void send_response(int sock, char response[BUF_SIZE]){
	int sf;
	
	sf = write(sock, response, BUF_SIZE);
	if(sf < 0){
		error("Couldn't send message");
	}
	
}

//Request info from server
void request_info(int sock){
	char bufferRead[BUF_SIZE], bufferSend[BUF_SIZE];
	int rb;

	rb = read(sock, bufferRead, BUF_SIZE);
	if(rb < 0){		
		error("Couldn't read message");	
	}else{
		bufferRead[rb] = '\0';
	}

	system("clear");

	printf("%s", bufferRead);

 	while(rb > 0){
 	 	fgets(bufferSend, BUF_SIZE ,stdin);
		
		write(sock, bufferSend, BUF_SIZE);
 		
		rb = read(sock, bufferRead, BUF_SIZE);
		bufferRead[rb] = '\0';	  	
		
		system("clear");
		printf("%s", bufferRead);
		
	}
}
