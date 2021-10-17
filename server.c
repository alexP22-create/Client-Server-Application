#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "helpers.h"
#include <netinet/tcp.h>
#include "list.h"

void usage(char *file)
{
	fprintf(stderr, "Usage: %s server_port\n", file);
	exit(0);
}

//inchide serverul si clientii udp
int exitServer(int socket_tcp, int socket_udp, fd_set read_fds, int fdmax) {
	char *command = malloc(5);
	fgets(command, 4, stdin);
	if (strcmp(command,"exit") == 0) {
		//trb inchisi toti clientii
		for (int i = 1; i <= fdmax; i++) {
			if ((i != socket_tcp) && (i != socket_udp) &&
				FD_ISSET(i, &read_fds)) {
					struct msgUDP msg;
					memset(&msg, 0, sizeof(struct msgUDP));
					strncpy(msg.payload, "exit", 4);
					int ret = send(i, &msg, sizeof(struct msgUDP), 0);
					DIE (ret < 0, "Send a dat gres\n");
				}
		}
		return 1;
	}
	return 0;
}

// gaseste un id din lista si seteaza clientul ca deconectat
char* find_id(struct client* clients, int socket) {
	struct client* aux = clients;
	while (aux != NULL) {
		if(aux->ord_socket_number == socket) {
			aux->isActive = 0;
			return aux->id;
		}
		aux = aux->next;
	}
}

//adauga un topic daca e cazul
void add_subscribed(struct client* clients, struct msgTCP msg) {
	//creare topic
	struct topic *newTopic = (struct topic*) malloc(sizeof(struct topic));
	
	//verif daca topicul se gaseste deja la clientul respectiv
	struct client* aux = clients;
	while (aux != NULL) {

		if (strcmp(aux->id, msg.id_client) == 0) {
			
			struct topic* aux2 = aux->topics;
			int already_exists = 0;

			while (aux2 != NULL) {
				//cand e deja abonat la topic
				if (strcmp(aux2->topic,msg.topic) == 0) {
					//setez doar sf-ul
					aux2->SF = msg.SF;
					aux2->isSubscribed = 1;
					already_exists = 1;
					break;
				}
				aux2 = aux2->next;
			}

			if (already_exists == 1)
				break;
			else {//adaug noul topic
				struct topic* newTopic = (struct topic*) malloc(sizeof(struct topic));
				newTopic->isSubscribed = 1;
				newTopic->next=NULL;
				memcpy(newTopic->topic, msg.topic, 50);
				newTopic->SF = msg.SF;
				pushTopic(newTopic, aux);
				
				break;

			}
			break;
		}
		aux = aux->next;
	} 
}

void unsubscribe(struct client* clients, struct msgTCP msg) {
	//nu scot din lista topicul, doar il setez la isSubscribed cu 0                  momentan
	//verif daca topicul se gaseste deja la clientul respectiv
	struct client* aux = clients;
	while (aux != NULL) {

		if (strcmp(aux->id,msg.id_client) == 0) {

			struct topic* aux2 = aux->topics;
			int done = 0;
			while (aux2 != NULL) {
				if (strcmp(aux2->topic,msg.topic) == 0) {
					aux2->isSubscribed = 0;
					done = 1;
					break;
				}

				aux2 = aux2->next;
			}
			if(done == 1)
				break;
		}
		aux = aux->next;
	}	
}

void send_to_clients(struct client* clients, struct msgUDP msg) {
	struct client* aux = clients;
	while (aux != NULL) {
		//cautare topic
		struct topic* aux2 = aux->topics;
		while (aux2 != NULL) {
			if (strcmp(aux2->topic, msg.topic) == 0) {
				
				//trimit pentru fiecare client conectat si abonat trimit mesajul
				if (aux2->isSubscribed == 1 && aux->isActive == 1) {
					
					int ret = send(aux->ord_socket_number, &msg, sizeof(struct msgUDP), 0);
					DIE(ret < 0, "send");
				}
				//pentru cei abonati dar deconectati salvez mesajul
				if (aux2->isSubscribed == 1 && aux->isActive == 0 && aux2->SF == 1){
					pushMsg(aux, msg);
				}
			}
			aux2 = aux2->next;
		}
		aux = aux->next;
	}
}

//reconecteaza un anumit client is ii trimite mesajele pierdute
int send_stored_msg(struct client* clients, char* id, struct sockaddr_in cli_addr, int new_socket) {
	struct client* aux = clients;

	while (aux != NULL) {
		if (strcmp(aux->id,id) == 0 && aux->isActive == 0) {
			
			printf("New client %s connected from %s:%d.\n", id, 
								inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port));
			struct topic* topic = aux->topics;
			while (topic != NULL) {
				struct TCelMsg *msg = aux->missedMsges;
				while (msg != NULL) {
					if (topic->isSubscribed == 1 && strcmp(topic->topic, msg->msg.topic) == 0
						&& topic->SF == 1 && msg->wasSend == 0) {
						struct msgUDP msg_to_send;
						memset(&msg_to_send, 0, sizeof(struct msgUDP));
						msg_to_send = msg->msg;
						int ret = send(aux->ord_socket_number, &msg_to_send, sizeof(msg_to_send), 0);
						//se marcheaza ca a fost deja trimis
						msg->wasSend = 1;
						DIE(ret < 0, "send");
					}
					msg = msg->next;
				}

				topic = topic->next;
			}
			//il activam
			aux->isActive = 1;
			aux->ord_socket_number = new_socket;
			return 1;
		}
		aux = aux->next;
	}
	return 0;
}

int main(int argc, char *argv[])
{
	setvbuf(stdout, NULL, _IONBF, BUFSIZ);
	//avem nevoie de socketuri si pt udp si tcp
	int socket_tcp, newsocket_tcp, portno;
	int socket_udp;
	char buffer[BUFLEN];
	memset(buffer, 0, BUFLEN);
	struct sockaddr_in serv_addr, cli_addr, udp_addr;
	int n, i, ret;
	socklen_t clilen;
	int flag = 1;

	fd_set read_fds;	// multimea de citire folosita in select()
	fd_set tmp_fds;		// multime folosita temporar
	int fdmax = 0;			// valoare maxima fd din multimea read_fds

	if (argc < 2) {
		usage(argv[0]);
	}

	// se goleste multimea de descriptori de citire (read_fds) si multimea temporara (tmp_fds)
	FD_ZERO(&read_fds);
	FD_ZERO(&tmp_fds);

	//socket pasiv folosit la TCP
	socket_tcp = socket(AF_INET, SOCK_STREAM, 0);
	DIE(socket_tcp < 0, "socket TCP");
	//socket pasiv folosit la UDP
	socket_udp = socket(AF_INET, SOCK_DGRAM, 0);
	DIE(socket_tcp < 0, "socket UDP");

	portno = atoi(argv[1]);
	DIE(portno == 0, "atoi");

	//setez ip-ul si portul
	memset((char *) &serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(portno);
	serv_addr.sin_addr.s_addr = INADDR_ANY;

	memset((char *) &udp_addr, 0, sizeof(udp_addr));
	udp_addr.sin_family = AF_INET;
	udp_addr.sin_port = htons(portno);
	udp_addr.sin_addr.s_addr = INADDR_ANY;

	//bind socket TCP
	ret = bind(socket_tcp, (struct sockaddr *) &serv_addr, sizeof(struct sockaddr));
	DIE(ret < 0, "bind TCP");
	//bind socket UDP
	ret = bind(socket_udp, (struct sockaddr*) &udp_addr, sizeof(struct sockaddr));
	DIE(ret < 0, "bind UDP");
	//se primesc conexiuni pe socketul TCP
	ret = listen(socket_tcp, MAX_CLIENTS);
	DIE(ret < 0, "listen");
	// se adauga noul file descriptor (socketul pe care se asculta conexiuni) in multimea read_fds
	FD_SET(socket_tcp, &read_fds);
	FD_SET(socket_udp, &read_fds);
	FD_SET(0, &read_fds);

	if(socket_tcp > socket_udp)
		fdmax = socket_tcp;
	else
		fdmax = socket_udp;

	//dezactivez Nagle
	int result = setsockopt(socket_tcp, IPPROTO_TCP, TCP_NODELAY, (char*) &flag, sizeof(int));
	DIE(result < 0, "setsockopt");

	//varful sirului de clienti, folosit ca baza de date
	struct client *clients = NULL;

	while (1) {
		tmp_fds = read_fds; 

		ret = select(fdmax + 1, &tmp_fds, NULL, NULL, NULL);
		
		DIE(ret < 0, "select");
		if (FD_ISSET(0, &tmp_fds)) {//cand primeste de la tastatura exit
			if (exitServer(socket_tcp, socket_udp, read_fds, fdmax) == 1) {
				break;
			}
		}
		
		for (i = 0; i <= fdmax; i++) {
			if (FD_ISSET(i, &tmp_fds)) {
				if (i == socket_tcp) {
					
					// a venit o cerere de conexiune pe socketul inactiv (cel cu listen),
					// pe care serverul o accepta
					clilen = sizeof(cli_addr);
					int new_socket_tcp = accept(socket_tcp, (struct sockaddr *) &cli_addr, &clilen);
					DIE(new_socket_tcp < 0, "accept");

					//dezactivare Nagle
					int result = setsockopt(socket_tcp, IPPROTO_TCP, TCP_NODELAY,
						(char *) &flag, sizeof(int));

					// se adauga noul socket intors de accept() la multimea descriptorilor de citire
					FD_SET(new_socket_tcp, &read_fds);
					int last_fdmax = fdmax;//in caz ca new_socket_tcp se va elimina
					if (new_socket_tcp > fdmax) { 
						fdmax = new_socket_tcp;
					}

					char id[BUFLEN];
					memset(id, 0, BUFLEN);
					//se primeste mesajul cu id-ul clientului ce vrea sa se conecteze
					ret = recv(new_socket_tcp, id, sizeof(struct msgTCP), 0);
					DIE(new_socket_tcp < 0, "client not received");


					//daca clientul conectat s-a reconectat si are SF 1 ii trimit mesajele pierdute
					int already_connected = send_stored_msg(clients, id, cli_addr,new_socket_tcp);

					if (already_connected == 0) {
						//verific daca id-ul clientului care vrea sa se conecteze se regaseste deja la un client conectat
						int unic_id = 1;
						
						struct client *aux = clients;
						while (aux != NULL) {
							//daca s-a gasit id-ul la un client deja conectat
							if(strcmp(id, aux->id) == 0 && aux->isActive == 1) {
								printf("Client %s already connected.\n", id);
								//inchid noul client
								struct msgUDP msg;
								memset(&msg, 0, sizeof(struct msgUDP));
								strncpy(msg.payload, "exit", 4);
								int ret = send(new_socket_tcp, &msg, sizeof(struct msgUDP), 0);
								DIE (ret < 0, "Send a dat gres\n");
								unic_id = 0;

								//elimin din mult de descriptori
								FD_CLR(new_socket_tcp, &read_fds);
								//se revine la fdmax trecut
								fdmax = last_fdmax;
								
								break;
							}
							aux = aux->next;
						}

						if(unic_id == 0) {
							continue;
						}
						else {
							//adaugam noul client in lista de clienti
							struct client * newClient = (struct client*) malloc(sizeof(struct client));
							strncpy(newClient->id, id, 10);
							newClient->ord_socket_number = new_socket_tcp;
							newClient->isActive = 1;
							newClient->next = NULL;
							if (clients == NULL) {
								clients = newClient;
							}
							else {
								struct client *aux = clients;
								while (aux->next != NULL)
									aux = aux->next;
								aux->next = newClient; 
							}
						}

						printf("New client %s connected from %s:%d.\n", id, 
								inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port));
					}
				}
				else if (i == socket_udp) {
					//am primit mesaje pe socketul udp de la clientii UDP
					socklen_t udplen = sizeof(struct sockaddr);
					struct msgUDP_Client msg_received;
					memset(&msg_received, 0, sizeof(struct msgUDP_Client));
					ret = recvfrom(socket_udp, &msg_received, sizeof(msg_received), 0,
						(struct sockaddr*) &udp_addr, &udplen);
					//printf("\n%s -> %s\n", msg_received.topic, msg_received.payload);
					DIE(ret < 0, "receive");

					//creare mesaj nou de trimis la clientii TCP
					struct msgUDP msg;
					memset(&msg, 0, sizeof(struct msgUDP));
					msg.data_type = msg_received.data_type;
					memcpy(msg.payload, msg_received.payload, 1500);
					memcpy(msg.topic, msg_received.topic, 50);
					strcpy(msg.udp_ip, inet_ntoa(udp_addr.sin_addr));
					msg.port = udp_addr.sin_port;

					//se trimite tuturor clientilor conectati la acest topic
					send_to_clients(clients, msg);
				} else {
					// s-au primit date pe unul din socketii de client,
					// asa ca serverul trebuie sa le receptioneze
					//se primeste mesaj de tipul TCP de la clienti
					struct msgTCP msg;
					memset(&msg, 0, sizeof(struct msgTCP));
					n = recv(i, &msg, sizeof(struct msgTCP), 0);
					DIE(n < 0, "recv");

					if (n == 0) {
						//gasire id client deconectat si setare camp deconectat
						char* idd = find_id(clients, i);

						// conexiunea s-a inchis
						printf("Client %s disconnected.\n", idd);
						close(i);
						
						// se scoate din multimea de citire socketul inchis 
						FD_CLR(i, &read_fds);

					} else { //se primesc mesaje de subscribe sau unsubscribe
						
						//subscribe
						if (strncmp(msg.command_type, "subscribe",9) == 0) {
							add_subscribed(clients, msg);
						}

						//unsubscribe
						if (strncmp(msg.command_type, "unsubscribe",11) == 0) {
							unsubscribe(clients, msg);
						}
					
					}
				}
			}
		}
	}
	
	//eliberare memorie
	struct client* aux = clients;
	while (aux != NULL) {
		//eliberare mesaje
		struct TCelMsg * msg = aux->missedMsges;
		while (msg != NULL) {
			struct TCelMsg* free_msg = msg;
			msg = msg->next;
			free(free_msg);
		}

		//eliberare topicuri
		struct topic* topic = aux->topics;
		while (topic != NULL) {
			struct topic* free_topic = topic;
			topic = topic->next;
			free(free_topic);
		}

		struct client* free_client = aux;
		aux = aux->next;
		free(free_client);
	}
	
	close(socket_tcp);
	close(socket_udp);

	return 0;
}