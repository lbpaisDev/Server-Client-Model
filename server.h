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
#include <curl/curl.h>
#include <json-c/json.h>

#define SERVER_NAME "ISABELA PRIVACY SERVER"
#define SERVER_PORT 9005
#define SERVER_PORT2 9090
#define BUF_SIZE 1024   

struct string {
	char *ptr;
	size_t len;
};

struct sockaddr_in c_addr, s_addr;

typedef struct uinfo{
	const char *ID;
	const char *type;
	const char *activity;
	const char *location;
	int callslen;
	int callsmade;
	int callsmissed;
	int callsrecv;
	const char *department;
	int smsrecv;
	int smssent;
}uinfo;

typedef struct ginfo{
	int avgcallslen;
	int avgcallsmade;
	int avgcallsmissed;
	int avgcallsrecv;
	int avgsmsrecv;
	int avgsmssent;
}ginfo;

void error(char *msg);
void cleanup(int signum);
int send_response(int sock, char response[BUF_SIZE]);
size_t writefunc(void *ptr, size_t size, size_t nmemb, struct string *s);
void init_string(struct string *s);
int check_ID(char ID[BUF_SIZE]);
void user_info(char ID[BUF_SIZE] , int sock, int posID);
void group_info(char ID[BUF_SIZE], int sock, int posID);
void user_info_menu(int sock, char ID[BUF_SIZE], int posID);
void group_info_menu(int sock, char ID[BUF_SIZE], int posID);
void subscriptions_menu(int sock, char ID[BUF_SIZE], int posID);
void main_menu(int sock, char ID[BUF_SIZE], int posID);
void login(int sock);
void check_updates(int sock);
void sub_server(struct sockaddr_in s_addr, struct sockaddr_in c_addr);
void server(struct sockaddr_in s_addr, struct sockaddr_in c_addr);

