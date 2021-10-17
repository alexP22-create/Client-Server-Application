Postolache Alexandru-Gabriel 321 CB

							README TEMA2 PC

	Atentie!!!:
	In cazul in care la rularea scriptului test.py se intampla sa apara eroare
la pornirea serverului sau timeout va rog sa modificati portul din test.py. S-a
discutat si pe forum ac lucru.

	Voi explica codul pe fisiere si functii:

	1) list.h
		Structura struct topic este o celula dintr-o lista care retine un topic.
		Structura struct TCelMsg este o celula dintr-o lista care retine un
mesaj ratat de un client cu sf 1.
		Cele 2 metode ramase sunt folosite pentru a adauga un element la finalul
unei liste.

	2) header.h
		Primele 3 structuri sunt pentru mesaje TCP, UDP primite de la client la
server si UDP trimise de la server la clientii TCP. Trb 2 structuri udb caci
avem nevoie sa transmitem mai multe campuri la clientii TCP.
		Structura struct client este folosita la stocarea informatiilor despre
un client TCP. Clientii se vor retine intr-o lista cu elemente de acest tip.

	3) subscriber.c
	Metode:
		* subscribe(), primeste comanda data clientului TPC si trimite o cerere
de subscribe la topicul respectiv serverului.
		* unsubscribe(), acelasi lucru doar ca trimite o cerere de unsubscribe.
		* execute_command(), primeste comanda si alege una din metodele de mai
sus.
		* printINT(), printFLOAT(), printSHORT_REAL(), printSTRING(), printeaza
mesajul facand conversie la payload.
		* main().
		Daca se citeste o comanda de la stdin se va executa. Daca se primeste
un mesaj de la server fie se va inchide clientul TCP in cazul primirii lui exit,
fie va printa payload-ul mesajul respectiv dupa forma specificata in enunt.
		* exitServer(), daca se primeste de la stdin comanda exit, metoda
va returna 1 abia dupa ce va trimite o comanda de inchidere tuturor clientilor
sai.
		* find_id(), cauta un client in lista de clienti si il seteaza ca
neconectat.
		* add_subscribed(), adauga un topic la un client in cazul in care
clientul n-a mai fost pana acum abonat la acel topic. Cand a fost deja abonat
ii actualizeaza campul de SF.
		* unsubscribe(), dezaboneaza un client de la un topic. La dezabonare
nu elimin topicul din lista de topicuri a clientului, doar ii setez campul
isSubscribed la 0 deoarece in cazul in care se da subscribe iar sa nu mai fac
iar obiectul, inserarea si restructurarea listei.
		* send_to_clients(), cauta in lista de clienti toti clientii abonati
la un topic si care sunt si conectati in acel moment si le trimite mesajul.
Daca sunt abonati dar neconectati atunci se va stoca mesajul intr-o lista
de mesaje din structura clientului.
		* send_stored_msg(), cauta sa vada daca clientul care vrea sa se
reconecteze acum a mai fost sau nu conectat inainte. Daca a mai fost, verifica
sa vada daca a avut vreo subscriptie cu SF 1 la vreun topic si in acel caz ii
trimite in ordine cronologica toate mesajele depozitate. Marcheaza mesajele
trimise ca trimise si reactiveaza clientul reactualizandu-i socketul.
		* main().
		Cand se primeste o cerere de conexiune pe socketul pasiv se verifica
daca a fost conectat in trecut, i se verifica unicitatea id-ului si eventual
se adauga in lista de clienti.
		Daca se primeste un mesaj pe socketul de udp, venit de la clientii
UDP, se creeaza noul mesaj pentru ruta server -> tcp client si se trimite la
clienti.
		Daca s-au primit mesajele cu comenzi de la clientii TCP se vor
indeplini cele de subscribe si unsubscribe. Daca nr de biti primit este 0
inseamna ca un client s-a inchis si se afiseaza mesajul. 
