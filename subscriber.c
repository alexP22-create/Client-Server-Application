#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "helpers.h"
#include "header.h"
#include <math.h>
#include <netinet/tcp.h>

void usage(char *file)
{
	fprintf(stderr, "Usage: %s server_address server_port\n", file);
	exit(0);
}

void subscribe(char* buffer, int sockfd, char*id) {
	char command[15], topic[50];
	char* token = strtok(buffer, " ");
	strcpy(command, token);

	token = strtok(NULL, " ");
	if (token != NULL)             
	 strcpy(topic, token);

	token = strtok(NULL, " ");
	char sf;
	if (token != NULL)
		sf = token[0] - '0';

	//trimitere mesaj catre server cu abonarea la topic
	if (sf == 1 || sf == 0){
		struct msgTCP msg;
		memset(&msg, 0, sizeof(struct msgTCP));
		memcpy(msg.command_type, command, 15);
		memcpy(msg.topic, topic, 50);
		strcpy(msg.id_client, id);
		msg.SF = sf;

		int bytesSend = send(sockfd, &msg, sizeof(msg), 0);
		DIE(bytesSend < 0, "subscribe message failed");
		printf("Subscribed to topic.\n");
	}
}

void unsubscribe(char* buffer, int sockfd, char*id){
	char command[15];
	char* token = strtok(buffer, " ");
	strcpy(command, token);

	char topic[50];
	token = strtok(NULL, " ");
	if (token != NULL)
		strcpy(topic, token);

	struct msgTCP msg;
	memset(&msg, 0, sizeof(struct msgTCP));
	memcpy(msg.command_type, command, 15);
	memcpy(msg.topic, topic, 50);
	strcpy(msg.id_client, id);
	msg.SF = 0;

	int bytesSend = send(sockfd, &msg, sizeof(msg), 0);
	DIE(bytesSend < 0, "unsubscribe message failed");
	printf("Unsubscribed from topic.\n");
}
//tells the client what kind of command it got and executes it
void execute_command(char* buffer, int sockfd, char* id){
	char copy_buffer[BUFLEN];
	strcpy(copy_buffer, buffer);
	char* token = strtok(buffer, " ");
	if (token != NULL) {
		if (strcmp(token, "subscribe") == 0) {
			subscribe(copy_buffer, sockfd, id);
			return;
		}
		else
			if (strcmp(token, "unsubscribe") == 0) {
				unsubscribe(copy_buffer, sockfd, id);
				return;
			}
	}
}
//prints the message after converting to INT
void printINT(struct msgUDP msg) {
	uint32_t value = ntohl(*(uint32_t*)(msg.payload + 1));
	
	//verif bit semnificativ
	if (msg.payload[0] != 0) {
		value = value * (-1);
	}
	printf("%s:%d - %s - INT - %d\n", msg.udp_ip, msg.port,
		msg.topic, value);
}

void printFLOAT(struct msgUDP msg) {
	float value = ntohl(*(uint32_t*)(msg.payload+1));
	value = value/pow(10, msg.payload[5]);
	if (msg.payload[0] == 1) {
		value = value * (-1);
	}
	printf("%s:%d - %s - FLOAT - %lf\n", msg.udp_ip, msg.port,
		msg.topic, value);

}

void printSHORT_REAL(struct msgUDP msg) {
	double value = ntohs(*(uint16_t*)(msg.payload));
    value = value / 100;
	printf("%s:%d - %s - SHORT_REAL - %.2f\n", msg.udp_ip, msg.port,
		msg.topic, value);
}

void printSTRING(struct msgUDP msg) {
	printf("%s:%d - %s - STRING - %s\n", msg.udp_ip, msg.port,
		msg.topic, msg.payload);	
}

int main(int argc, char *argv[])
{	
	setvbuf(stdout, NULL, _IONBF, BUFSIZ);
	int sockfd, n, ret;
	int flag = 1;
	struct sockaddr_in serv_addr;
	char buffer[BUFLEN];

	if (argc < 3) {
		usage(argv[0]);
	}

	//declarare multime cu descriptori rezultati de la select
	fd_set read_fds, tmp_fds;
	FD_ZERO(&read_fds);
	FD_ZERO(&tmp_fds);

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	DIE(sockfd < 0, "socket");

	FD_SET(sockfd, &read_fds);
	FD_SET(0, &read_fds);


	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(atoi(argv[3]));
	ret = inet_aton(argv[2], &serv_addr.sin_addr);
	DIE(ret == 0, "inet_aton");

	ret = connect(sockfd, (struct sockaddr*) &serv_addr, sizeof(serv_addr));
	DIE(ret < 0, "connect");

	int nfds = sockfd;//valoarea maxima a unui elem din multime

	//dezactivez Nagle
	int result = setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, (char*) &flag, sizeof(int));
	DIE(result < 0, "setsockopt");

	//trimitere id-ul clientului curent TCP
	ret = send(sockfd, argv[1], strlen(argv[1]) + 1, 0);
	DIE(ret < 0, "No Client");

	while (1) {
  		tmp_fds = read_fds;
		//lista de descriptori
		ret = select(nfds + 1, &tmp_fds, NULL, NULL, NULL);
		DIE(ret < 0, "select");

		if (FD_ISSET(0, &tmp_fds)) {
			//se citeste de la stdin
			memset(buffer, 0, BUFLEN);
			fgets(buffer, BUFLEN - 1, stdin);
			
			if (strncmp(buffer,"exit", 4) == 0)
				break;
			execute_command(buffer, sockfd, argv[1]);
		}
		if (FD_ISSET(sockfd, &tmp_fds)) {
			//primeste mesaj de la server
			struct msgUDP msg;
			memset(&msg, 0, sizeof(struct msgUDP));
			int lengthMsg = recv(sockfd, &msg, sizeof(struct msgUDP), 0);
			
			if (lengthMsg == 0)
				break;
			DIE(lengthMsg < 0, "Didn't recv from client udp");

			//daca primeste comanda exit
			if (strncmp(msg.payload, "exit", 4) == 0)
				break;
			
			//afisarea mesajului primit
			if (msg.data_type == 0) {
				printINT(msg);
			}
			if (msg.data_type == 1) {
				printSHORT_REAL(msg);
			}
			if (msg.data_type == 2) {
				printFLOAT(msg);
			}
			if (msg.data_type == 3) {
				printSTRING(msg);
			}						
		}
	}

	close(sockfd);

	return 0;
}