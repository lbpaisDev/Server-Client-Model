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
#include </home/lbpais/Documents/Projetos/ProjectIRC/server.h>                             

 /*
	Error detection
	Print error message
	Exit -1
*/
void error(char *msg){
	printf("\t-----ERROR-----\n%s\n", msg);
	exit(-1);
}

/*
	Receive CTRL_C 
	End conncetions
	Terminate processes
	Exit
*/
int sock, c_sock;
void cleanup(int signum){
	close(sock);
	close(c_sock);
	printf("\nReceived CTRL+C\nCleaning up...\nGoodbye!\n");
	while(wait(NULL) != -1);
	exit(0);
}

/*
	Send server response
	Error detection
	Return bytes sent
*/ 
int send_response(int sock, char response[BUF_SIZE]){
	int bs; 
	bs = write(sock, response, BUF_SIZE); 
	if(bs < 0){
		error("Couldn't send message");
	}
	return bs;
}

/*
	CODE FROM GITHUB
	TO INTERACT WITH 
	THE API
*/
//Write function to write the payload response in the string structure
size_t writefunc(void *ptr, size_t size, size_t nmemb, struct string *s)
{
	size_t new_len = s->len + size*nmemb;
	s->ptr = realloc(s->ptr, new_len + 1);
	if (s->ptr == NULL) {
		fprintf(stderr, "realloc() failed\n");
		exit(EXIT_FAILURE);
	}
	memcpy(s->ptr + s->len, ptr, size*nmemb);
	s->ptr[new_len] = '\0';
	s->len = new_len;

	return size*nmemb;
}

//Initilize the payload string
void init_string(struct string *s) {
	s->len = 0;
	s->ptr = malloc(s->len + 1);
	if (s->ptr == NULL) {
		fprintf(stderr, "malloc() failed\n");
		exit(EXIT_FAILURE);
	}
	s->ptr[0] = '\0';
}

//Get the Data from the API and return a JSON Object
struct json_object *get_student_data(){
	struct string s;
	struct json_object *jobj;

	//Intialize the CURL request
	CURL *hnd = curl_easy_init();

	//Initilize the char array (string)
	init_string(&s);

	curl_easy_setopt(hnd, CURLOPT_CUSTOMREQUEST, "GET");
	//To run on department network uncomment this request and comment the other
	//curl_easy_setopt(hnd, CURLOPT_URL, "http://10.3.4.75:9014/v2/entities?options=keyValues&type=student&attrs=activity,calls_duration,calls_made,calls_missed,calls_received,department,location,sms_received,sms_sent");
        //To run from outside
	curl_easy_setopt(hnd, CURLOPT_URL, "http://socialiteorion2.dei.uc.pt:9014/v2/entities?options=keyValues&type=student&attrs=activity,calls_duration,calls_made,calls_missed,calls_received,department,location,sms_received,sms_sent&limit=1000");

	//Add headers
	struct curl_slist *headers = NULL;
	headers = curl_slist_append(headers, "cache-control: no-cache");
	headers = curl_slist_append(headers, "fiware-servicepath: /");
	headers = curl_slist_append(headers, "fiware-service: socialite");

	//Set some options
	curl_easy_setopt(hnd, CURLOPT_HTTPHEADER, headers);
	curl_easy_setopt(hnd, CURLOPT_WRITEFUNCTION, writefunc); //Give the write function here
	curl_easy_setopt(hnd, CURLOPT_WRITEDATA, &s); //Give the char array address here

	//Perform the request
	CURLcode ret = curl_easy_perform(hnd);
	if (ret != CURLE_OK){
		fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(ret));

		/*jobj will return empty object*/
		jobj = json_tokener_parse(s.ptr);

		/* always cleanup */
		curl_easy_cleanup(hnd);
		return jobj;

	}
	else if (CURLE_OK == ret) {
		jobj = json_tokener_parse(s.ptr);
		free(s.ptr);

		/* always cleanup */
		curl_easy_cleanup(hnd);
		return jobj;
	}

}
/*
	END OF GITHUB CODE
*/


/*
	Receive ID
	Check database for received ID
	If not found return -1
	If found return pos of ID
*/
int check_ID(char ID[BUF_SIZE]){
	//Structs to hold fetched info
	struct json_object *jobj_array, *jobj_obj;
	struct json_object *jobj_object_id;
	enum json_type type = 0;
	int arraylen = 0, i; 
	const char *dbID; 
	
	//Fetch info
	jobj_array = get_student_data();
	arraylen = json_object_array_length(jobj_array);

	//Iterate array of fetched info
	for(i = 0; i < arraylen; i++){
		//Get i-th object in jobj_array
		jobj_obj = json_object_array_get_idx(jobj_array, i);

		//Get ID
		jobj_object_id = json_object_object_get(jobj_obj, "id");
		dbID = json_object_get_string(jobj_object_id);
		
		//If ID provided matches and ID in the database return 1
		if(strcmp(ID, dbID) == 0){
			return i;
		}
	}
	
	//Else return -1
	return -1; 
}

void  user_info_menu(int sock, char ID[BUF_SIZE], int posID){
	struct json_object *jobj_array, *jobj_obj;
	struct json_object *jobj_object_id, *jobj_object_type, *jobj_object_activity, *jobj_object_location, *jobj_object_latlong, *jobj_object_callsduration, 
	*jobj_object_callsmade, *jobj_object_callsmissed, *jobj_object_callsreceived, *jobj_object_department, *jobj_object_smsreceived, *jobj_object_smssent;
	enum json_type type = 0;
	int arraylen = 0, rb;
	char bufferWrite[BUF_SIZE], bufferRead[BUF_SIZE];

	//Fetch info
	jobj_array = get_student_data();
	arraylen = json_object_array_length(jobj_array);

	//get the name attribute in the i-th object
	jobj_obj = json_object_array_get_idx(jobj_array, posID);

	jobj_object_id = json_object_object_get(jobj_obj, "id");
	jobj_object_type = json_object_object_get(jobj_obj, "type");
	jobj_object_activity = json_object_object_get(jobj_obj, "activity");
	jobj_object_location = json_object_object_get(jobj_obj, "location");
	jobj_object_callsduration = json_object_object_get(jobj_obj, "calls_duration");
	jobj_object_callsmade = json_object_object_get(jobj_obj, "calls_made");
	jobj_object_callsmissed = json_object_object_get(jobj_obj, "calls_missed");
	jobj_object_callsreceived= json_object_object_get(jobj_obj, "calls_received");
	jobj_object_department = json_object_object_get(jobj_obj, "department");
	jobj_object_smsreceived = json_object_object_get(jobj_obj, "sms_received");
	jobj_object_smssent = json_object_object_get(jobj_obj, "sms_sent");

	sprintf(bufferWrite,"ID: %s\nType: %s\nActivity: %s\nLocation: %s\nCalls duration: %d\nCalls made: %d\nCalls missed: %d\nCalls received: %d\nDepartment: %s\nSMS received: %d\nSMS sent: %d\n\n[1] - Main menu\nChoose option: ",
	json_object_get_string(jobj_object_id), json_object_get_string(jobj_object_type),
	json_object_get_string(jobj_object_activity), json_object_get_string(jobj_object_location),
	atoi(json_object_get_string(jobj_object_callsduration)), atoi(json_object_get_string(jobj_object_callsmade)),
	atoi(json_object_get_string(jobj_object_callsmissed)), atoi(json_object_get_string(jobj_object_callsreceived)),
	json_object_get_string(jobj_object_department), atoi(json_object_get_string(jobj_object_smsreceived)),
	atoi(json_object_get_string(jobj_object_smssent)));

	send_response(sock, bufferWrite);
	
	rb = read(sock, bufferRead, BUF_SIZE);
	bufferRead[rb] = '\0';
	//Remove \n
	bufferRead[strlen(bufferRead) - 1] = '\0';
	
	if(atoi(bufferRead) == 1){
		main_menu(sock, ID, posID);		
	}else{
		user_info_menu(sock, ID, posID);
	}	
}

void group_info_menu(int sock, char ID[BUF_SIZE], int posID){
	//JSON obect
	struct json_object *jobj_array, *jobj_obj;
	struct json_object *jobj_object_id, *jobj_object_type, *jobj_object_activity, *jobj_object_location, *jobj_object_latlong, *jobj_object_callsduration, 
	*jobj_object_callsmade, *jobj_object_callsmissed, *jobj_object_callsreceived, *jobj_object_department, *jobj_object_smsreceived, *jobj_object_smssent;
	enum json_type type = 0;
	int arraylen = 0, i, rb;
	char bufferRead[BUF_SIZE], bufferWrite[BUF_SIZE];
	double sumcallslen = 0, sumcallsmade = 0, sumcallsmissed = 0, sumcallsrecv = 0, sumsmsrecv = 0, sumsmssent = 0;

	//Get the student data
	jobj_array = get_student_data();

	//Get array length
	arraylen = json_object_array_length(jobj_array);

	//Example of howto retrieve the data
	for (i = 0; i < arraylen; i++) {
		//get the i-th object in jobj_array
		jobj_obj = json_object_array_get_idx(jobj_array, i);

		//get the name attribute in the i-th object
		jobj_object_id = json_object_object_get(jobj_obj, "id");
		jobj_object_type = json_object_object_get(jobj_obj, "type");
		jobj_object_activity = json_object_object_get(jobj_obj, "activity");
		jobj_object_location = json_object_object_get(jobj_obj, "location");
		jobj_object_callsduration = json_object_object_get(jobj_obj, "calls_duration");
		jobj_object_callsmade = json_object_object_get(jobj_obj, "calls_made");
		jobj_object_callsmissed = json_object_object_get(jobj_obj, "calls_missed");
		jobj_object_callsreceived= json_object_object_get(jobj_obj, "calls_received");
		jobj_object_department = json_object_object_get(jobj_obj, "department");
		jobj_object_smsreceived = json_object_object_get(jobj_obj, "sms_received");
		jobj_object_smssent = json_object_object_get(jobj_obj, "sms_sent");

		if(json_object_get_string(jobj_object_callsduration) == NULL ){
			continue;
		}

		sumcallslen += atoi(json_object_get_string(jobj_object_callsduration));
		sumcallsmade += atoi(json_object_get_string(jobj_object_callsmade));
		sumcallsmissed += atoi(json_object_get_string(jobj_object_callsmissed));
		sumcallsrecv += atoi(json_object_get_string(jobj_object_callsreceived));
		sumsmsrecv += atoi(json_object_get_string(jobj_object_smsreceived));
		sumsmssent += atoi(json_object_get_string(jobj_object_smssent));
	}

	sprintf(bufferWrite,"Calls duration: %f\nCalls made: %f\nCalls missed: %f\nCalls received: %f\nSMS received: %f\nSMS sent: %f\n\n[1] - Main menu\nChoose option: ",
	sumcallslen/i, sumcallsmade/i, sumcallsmissed/i, sumcallsrecv/i, sumsmsrecv/i,
	sumsmssent/i);

	send_response(sock, bufferWrite);
	
	rb = read(sock, bufferRead, BUF_SIZE);
	bufferRead[rb] = '\0';
	//Remove \n
	bufferRead[strlen(bufferRead) - 1] = '\0';
	
	if(atoi(bufferRead) == 1){
		main_menu(sock, ID, posID);		
	}else{
		group_info_menu(sock, ID, posID);
	}

}

void subscriptions_menu(int sock, char ID[BUF_SIZE], int posID){
	char bufferRead[BUF_SIZE];
	int rb;

	send_response(sock, "Subscription info\n[1] - Main menu\nChoose option: ");
	
	rb = read(sock, bufferRead, BUF_SIZE);
	bufferRead[rb] = '\0';
	//Remove \n
	bufferRead[strlen(bufferRead) - 1] = '\0';
	
	if(atoi(bufferRead) == 1){
		main_menu(sock, ID, posID);		
	}else{
		subscriptions_menu(sock,ID, posID);
	}
}

/*
	Menu 
	Choose wich info to see
	(user or group)
*/
void main_menu(int sock, char ID[BUF_SIZE], int posID){
	char bufferRead[BUF_SIZE], bufferWrite[BUF_SIZE];
	int opt, rb;
	//Menu
	send_response(sock,"\t// MAIN MENU //\n--------------------------\n[1] - PRIVATE INFORMATION\n[2] - GROUP INFORMATION\n[3] - SUBSCRIPTION\n[4] - EXIT\nChoose option: ");

	//Options
	while(1){
		rb = read(sock, bufferRead, BUF_SIZE);
		bufferRead[rb] = '\0';
		bufferRead[strlen(bufferRead) - 1] = '\0';
		opt = atoi(bufferRead);
		//Navigation
		switch(opt){
			case 1:
				user_info_menu(sock, ID, posID);
				break;
			case 2:
				group_info_menu(sock, ID, posID);
				break;
			case 3:
				subscriptions_menu(sock, ID, posID);
				break;
			case 4:
				printf("Ended connection to %s\n", ID);
				sprintf(bufferWrite, "Leaving...\nGoodbye!");
				send_response(sock,bufferWrite);
				break;
			default:
				main_menu(sock, ID, posID);
		}
		break;
	}
	close(sock);
}

/*
	Receive and validate ID
	N tries to give a valid ID
*/
void login(int sock){
	char bufferRead[BUF_SIZE], bufferWrite[BUF_SIZE];
	int rb, n = 5, posID;

	//Send greetings
	sprintf(bufferWrite,"Hello...\nPlease insert you're ID:\n");
	send_response(sock, bufferWrite);
	
	for(int try = 1; try <= n; try++){
		rb = read(sock, bufferRead, BUF_SIZE);
		bufferRead[rb] = '\0';
		//Remove \n
		bufferRead[strlen(bufferRead) - 1] = '\0';
		//ID valid, go to options menu
		posID = check_ID(bufferRead);
		if(posID > 0 ){
			printf("Received registered ID(%s)\n",bufferRead);
			main_menu(sock,bufferRead, posID);
			break;
		//ID is not valid
		}else{
			printf("Received unregistered ID(%s)\n",bufferRead);
			sprintf(bufferWrite,"Sorry...\nTry again\n%d tries left\n",(n-try));
			send_response(sock,bufferWrite);		
		}	
		//No more tries
		if((n-try) == 0){
			send_response(sock,"No tries left!\nGoodbye...");
			break;	
		}
	}
	close(sock);
}

//Check for updates intervals of x seconds 
//Notify client if subscribed option have changed
void check_updates(int sock){

}

//Subscription server
void sub_server(struct sockaddr_in s_addr, struct sockaddr_in c_addr){
	char ip[BUF_SIZE];
	int c_size;

	//Subscription server port
	s_addr.sin_port = htons(SERVER_PORT2);

	//Create socket
	if((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0){
		//Error detection
		error("Couldn't create socket");
	}

	//Bind server to socket
	if(bind(sock,(struct sockaddr*)&s_addr,sizeof(s_addr)) < 0){
		//Error detection
		error("Couldn't bind server to socket");
	}

	//Listen for clients in the server socket previously binded
	if(listen(sock,5) < 0){
		//Error detection
		error("Couldn't listen to clients");
	}

	//Server label
	printf("WebServer: Subscription server running...\n");

	//Redirect SIGINT to CRL_C function
	signal(SIGINT, cleanup);

	//Server itself
 	while (1) {
 		//Clear processes that might have became zombies and are eating resources
    	while(waitpid(-1,NULL,WNOHANG)>0); 
    	
    	//Socket created for acception a connection from a client
    	c_size = sizeof(c_addr);
    	c_sock= accept(sock,(struct sockaddr *)&c_addr,&c_size);
    	
    	//Error detection
    	if (c_sock > 0) {
    		//Create process to handle client
      		if (fork() == 0) {
      			//Close server socket
      			close(sock);
      			//Handles subscriptions updates
      			check_updates(c_sock);
				//Terminate process
				exit(0);
      		} 
   		//Close client-server socket
   		close(c_sock);
   	 	}else{	
   	 		error("Couldn't accept client");
   	 	}
  	}
}

//Main Server
void server(struct sockaddr_in s_addr, struct sockaddr_in c_addr){
	char ip[BUF_SIZE];
	int c_size;

	//Main server port
	s_addr.sin_port = htons(SERVER_PORT);

	//Create socket
	if((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0){
		//Error detection
		error("Couldn't create socket");
	}

	//Bind server to socket
	if(bind(sock,(struct sockaddr*)&s_addr,sizeof(s_addr)) < 0){
		//Error detection
		error("Couldn't bind server to socket");
	}

	//Listen for clients in the server socket previously binded
	if(listen(sock,5) < 0){
		//Error detection
		error("Couldn't listen to clients");
	}

	//Display server info
	printf("\t-----%s-----\t\n",SERVER_NAME);
	printf("WebServer: Waiting for connections on port %d...\n", SERVER_PORT);

	//Redirect SIGINT to CRL_C function
	signal(SIGINT, cleanup);

	//Server itself
 	while (1) {
 		//Clear processes that might have became zombies and are eating resources
    	while(waitpid(-1,NULL,WNOHANG)>0); 
    	//Socket created for acception a connection from a client
    	c_size = sizeof(c_addr);
    	c_sock= accept(sock,(struct sockaddr *)&c_addr,&c_size);
    	
    	//Error detection
    	if (c_sock > 0) {
    		//Create process to handle client
      		if (fork() == 0) {
      			//Close Server socket
      			close(sock);
      			//Display client IPv4
      			inet_ntop(AF_INET,(struct sockaddr*)&c_addr.sin_addr, ip, BUF_SIZE);
				printf("WebServer: got connection from %s\n", ip);
				//Isabela Privacy Server Menu
				login(c_sock);
				//Terminate process
				exit(0);
      		}
   		//Close client-server socket
   		close(c_sock);
   	 	}else{
   	 		error("Couldn't accept client");   	 	
   	 	}
  	}
}

int main(void){
	//Variables used sock stands for socket, s for server, c client, addr address
	int c_size;
	struct sockaddr_in s_addr, c_addr;
	pid_t pid;
	int sock, c_sock;

	//Creates an IPv4 with zeros such as:
	//0.0.0.0
 	bzero((void *) &s_addr, sizeof(s_addr));
  	s_addr.sin_family = AF_INET;
  	s_addr.sin_addr.s_addr = htonl(INADDR_ANY);

  	system("clear");

  	//Child process handles main server
 	if((pid = fork()) == 0){
 		server(s_addr,c_addr);
 	//Error detection	
 	}else if(pid < 0){
 		error("Couldn't create process");
 	//Parent process handles sub server
 	}else{
 		sleep(1);
 		sub_server(s_addr, c_addr);
 	}
 	
	return 0;
}
