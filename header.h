#include <stdio.h>
#include <unistd.h>
#include <inttypes.h>
#include <string.h>

/* Mesaj de tip tcp pe ruta client->server */
struct msgTCP {
	char command_type[15];		// Tipul comenzii 
	char topic[50];				// Titlul topicului 
	char id_client[10];				// Id-ul clientului 
	unsigned int SF;					// parametrul store & forward
};

/* Mesaj de tip UDP primit de server de la clientii udp */
struct msgUDP_Client {
	char topic[50];			// titlul topicului 
	unsigned char data_type;		// tipul de date 
	char payload[1500];
};

/* Mesaj de tip UDP pe ruta server->client tcp */
struct msgUDP {
	char udp_ip[12];			// Ip-ul clientului UDP 
	int port;						// Port-ul cientului UDP 
	char topic[50];			// titlul topicului 
	unsigned char data_type;		// tipul de date 
	char payload[1500];
};

/* structura pentru clientii TCP ai serverului */
struct client {
	char id[10];				// id-ul clientului
	int ord_socket_number;     // indexul socketului
	int isActive;				//the client's status
	struct topic *topics;		//topicurile la care e abonat
	struct TCelMsg *missedMsges;	//mesaje pierdute
	struct client* next;
};
