#include <stdio.h>
#include <stdlib.h>
#include "header.h"

/* celula dintr-o lista care retine topicurile */
struct topic {
	struct topic *next;	
	char topic[50];				//numele topicului
	int isSubscribed;			// daca clientul este in ac moment abonat la topic
	unsigned int SF;			// tipul de abonare al clientului
};

/* celula dintr-o lista care retine mesajele pierdute */
struct TCelMsg {
	struct TCelMsg *next;
	struct msgUDP msg;		//mesajul in sine
	int wasSend;			//ne spune daca a fost trimis deja
};
// se adauga in lista de topicuri a unei structuri client
void pushTopic (struct topic *topic, struct client *client) { 
	
	if(client->topics == NULL)
		client->topics = topic;
	else {
		struct topic *aux = client->topics;
		while (aux->next != NULL)
			aux = aux->next;
		aux->next = topic;
	}
}

// se adauga in lista de mesaje netrimise a unei structuri de tip client cu SF 1
void pushMsg(struct client *client, struct msgUDP msg) { 
	struct TCelMsg *newCel = (struct TCelMsg*) malloc(sizeof(struct TCelMsg));

	newCel->msg = msg;
	newCel->next = NULL;
	
	if (client->missedMsges == NULL)
		client->missedMsges = newCel;
	else {
		struct TCelMsg *aux = client->missedMsges;
		while(aux->next != NULL)
			aux = aux->next;
		aux->next = newCel;
	}
}
