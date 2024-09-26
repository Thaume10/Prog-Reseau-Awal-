#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>
#include <pthread.h>
#include <unistd.h>
#include <jansson.h>
#include "server2.h"
#include "client2.h"

// TODO historique, si joueur se déco pendant la conncetion (voir comment ping serveur) + pendant la partie, reco, stream, chat,bio

Client clients[MAX_CLIENTS];
Client chat[MAX_CLIENTS];
Defi defis[MAX_CLIENTS];
int nbDefi = 0;
int nbChat = 0;
fd_set rdfs;
int actual = 0;

pthread_mutex_t mutexClient = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutexDefi = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutexChat = PTHREAD_MUTEX_INITIALIZER;

static void init(void)
{
#ifdef WIN32
	WSADATA wsa;
	int err = WSAStartup(MAKEWORD(2, 2), &wsa);
	if (err < 0)
	{
		puts("WSAStartup failed !");
		exit(EXIT_FAILURE);
	}
#endif
}

static void end(void)
{
#ifdef WIN32
	WSACleanup();
#endif
}

static void app(void)
{
	SOCKET sock = init_connection();
	char buffer[BUF_SIZE];
	int max = sock;
	pthread_mutex_lock(&mutexDefi);

	// chargerDefis("donnees.json");
	pthread_mutex_unlock(&mutexDefi);

	while (1)
	{
		int i = 0;
		FD_ZERO(&rdfs);

		/* add STDIN_FILENO */
		FD_SET(STDIN_FILENO, &rdfs);

		/* add the connection socket */
		FD_SET(sock, &rdfs);

		/* add socket of each client */
		pthread_mutex_lock(&mutexClient);
		for (i = 0; i < actual; i++)
		{
			FD_SET(clients[i].sock, &rdfs);
		}
		pthread_mutex_unlock(&mutexClient);

		if (select(max + 1, &rdfs, NULL, NULL, NULL) == -1)
		{
			perror("select()");
			saveJSON();
			exit(errno);
		}

		/* something from standard input : i.e keyboard */
		if (FD_ISSET(STDIN_FILENO, &rdfs))
		{
			/* stop process when type on keyboard */
			break;
		}
		else if (FD_ISSET(sock, &rdfs))
		{
			/* new client */
			SOCKADDR_IN csin = {0};
			size_t sinsize = sizeof csin;
			int csock = accept(sock, (SOCKADDR *)&csin, &sinsize);
			if (csock == SOCKET_ERROR)
			{
				perror("accept()");
				continue;
			}

			/* after connecting the client sends its name */
			if (read_client(csock, buffer) == -1)
			{
				/* disconnected */
				continue;
			}

			/* what is the new maximum fd ? */
			max = csock > max ? csock : max;

			FD_SET(csock, &rdfs);
			pthread_mutex_lock(&mutexClient);
			Client c = {csock};
			strncpy(c.name, buffer, BUF_SIZE - 1);
			c.dispo = 1;
			clients[actual] = c;
			actual++;

			/* Send a message to all clients */
			char connectMessage[BUF_SIZE];
			snprintf(connectMessage, BUF_SIZE, "%s s'est connecté.e!", c.name);

			send_message_to_all_clients(clients, c, actual, connectMessage, 1);
			pthread_t thread_id;
			pthread_create(&thread_id, NULL, fct_client, &clients[actual - 1]);
			pthread_detach(thread_id);
			pthread_mutex_unlock(&mutexClient);
		}
	}
	pthread_mutex_lock(&mutexClient);
	saveJSON();
	clear_clients(clients, actual);
	end_connection(sock);
	pthread_mutex_unlock(&mutexClient);
}

void chargerDefis(const char *nomFichier)
{
	json_t *root;
	json_error_t error;

	root = json_load_file(nomFichier, 0, &error);
	if (!root)
	{
		fprintf(stderr, "Erreur de chargement du fichier JSON : %s\n", error.text);
		return;
	}

	if (json_is_object(root))
	{
		json_t *defisJSON = json_object_get(root, "defis");
		if (json_is_array(defisJSON))
		{
			for (size_t i = 0; i < json_array_size(defisJSON); i++)
			{
				Defi defi;
				defi.nbViewers = 0;
				Client c1, c2;
				json_t *defiJSON = json_array_get(defisJSON, i);

				json_t *status = json_object_get(defiJSON, "status");
				defi.status = json_integer_value(status);

				json_t *joueurs = json_object_get(defiJSON, "joueurs");
				const char *joueur1JSON = json_string_value(json_array_get(joueurs, 0));
				const char *joueur2JSON = json_string_value(json_array_get(joueurs, 1));
				strncpy(c1.name, joueur1JSON, sizeof(c1.name) - 1);
				c1.name[sizeof(c1.name) - 1] = '\0';
				strncpy(c2.name, joueur2JSON, sizeof(c2.name) - 1);
				c2.name[sizeof(c2.name) - 1] = '\0';

				defi.clientDefie = &c1;
				defi.clientQuiDefie = &c2;

				Partie p;

				json_t *partie = json_object_get(defiJSON, "partie");

				json_t *plateau = json_object_get(partie, "plateau");
				for (int i = 0; i < 12; i++)
				{
					p.plateau[i] = json_integer_value(json_array_get(joueurs, i));
				}

				json_t *sens = json_object_get(partie, "sens");
				p.sens = json_integer_value(sens);

				json_t *points = json_object_get(partie, "points");
				p.points[0] = json_integer_value(json_array_get(points, 0));
				p.points[1] = json_integer_value(json_array_get(points, 1));

				json_t *joueurActuel = json_object_get(partie, "joueurActuel");
				p.joueurActuel = json_integer_value(joueurActuel);
				defi.partie = &p;
				defis[nbDefi] = defi;
				nbDefi++;
			}
		}
	}

	json_decref(root);
	for (int i = 0; i < nbDefi; i++)
	{
		printf("Status: %d\n", defis[i].status);
		printf("Joueur 1: %s\n", defis[i].clientDefie->name);
		printf("Joueur 2: %s\n", defis[i].clientQuiDefie->name);
		printf("Sens: %d\n", defis[i].partie->sens);
		printf("Points Joueur 1: %d\n", defis[i].partie->points[0]);
		printf("Points Joueur 2: %d\n", defis[i].partie->points[1]);
		printf("Joueur Actuel: %d\n", defis[i].partie->joueurActuel);
	}
}

static void saveJSON()
{

	FILE *fichier = fopen("donnees.json", "w");

	if (fichier == NULL)
	{
		perror("Erreur lors de l'ouverture du fichier.");
		return 1;
	}

	// Commencez à construire le JSON
	fprintf(fichier, "{\n");
	fprintf(fichier, "  \"defis\": [\n");

	for (int i = 0; i < nbDefi; i++)
	{
		if (defis[i].status == 2)
		{
			defis[i].status = 3;
		}
		fprintf(fichier, "    {\n");
		fprintf(fichier, "      \"status\": %d,\n", defis[i].status);
		fprintf(fichier, "      \"joueurs\": [\"%s\", \"%s\"],\n", defis[i].clientDefie->name, defis[i].clientQuiDefie->name);
		fprintf(fichier, "      \"partie\": {\n");
		fprintf(fichier, "        \"plateau\": [");
		for (int j = 0; j < TAILLE_PLATEAU; j++)
		{
			fprintf(fichier, "%d", defis[i].partie->plateau[j]);
			if (j < TAILLE_PLATEAU - 1)
				fprintf(fichier, ", ");
		}
		fprintf(fichier, "],\n");
		fprintf(fichier, "        \"sens\": %d,\n", defis[i].partie->sens);
		fprintf(fichier, "        \"points\": [%d, %d],\n", defis[i].partie->points[0], defis[i].partie->points[1]);
		fprintf(fichier, "        \"joueurActuel\": %d\n", defis[i].partie->joueurActuel);
		fprintf(fichier, "      }\n");

		if (i < nbDefi - 1)
			fprintf(fichier, "    },\n");
		else
			fprintf(fichier, "    }\n");
	}

	fprintf(fichier, "  ]\n");
	fprintf(fichier, "}\n");

	// Fermez le fichier
	fclose(fichier);
}

static void *fct_client(void *client)
{
	Client *client_ptr = (Client *)client;
	char buffer[BUF_SIZE];
	int lancerdefi = 0;
	int recepDefi = 0;
	int enChat = 0;
	int streaming = 0;
	int choix = -1;
	char message[BUF_SIZE];
	Defi *meDefi;

	if (FD_ISSET(client_ptr->sock, &rdfs))
	{
		snprintf(message, BUF_SIZE, "BOUJOUR %s\nTappe 1 si tu veux jouer, tappe 2 si tu veux regarder une partie, tappe 3 pour voir l'historique des parties, tappe 4 pour participer au chat general", client_ptr->name);
		write_client(client_ptr->sock, message);

		while (1)
		{

			// snprintf(message, BUF_SIZE, "\ntest dispo  %d   n", client_ptr->dispo);
			// write_client(client_ptr->sock, message);
			int c = read_client(client_ptr->sock, buffer);
			if (c == 0)
			{
				pthread_mutex_lock(&mutexDefi);
				for (int i = 0; i < nbDefi; i++)
				{
					if (defis[i].status == 1 || defis[i].status == 2)
					{
						if (defis[i].clientQuiDefie->sock == client_ptr->sock || defis[i].clientDefie->sock == client_ptr->sock)
						{
							// Trouvé un défi actif impliquant le joueur déconnecté
							defis[i].status = 3; // Statut interrompu
							recepDefi = 0;
							snprintf(message, BUF_SIZE, "Votre adversaire s'est déconnecté. Tapez 'menu' pour revenir au menu\n");
							write_client((client_ptr->sock == defis[i].clientQuiDefie->sock) ? defis[i].clientDefie->sock : defis[i].clientQuiDefie->sock, message);
							break; // Vous pouvez sortir de la boucle après avoir trouvé le défi actif
						}
					}
				}
				pthread_mutex_unlock(&mutexDefi);
				int place;
				pthread_mutex_lock(&mutexClient);
				for (int i = 0; i < actual; i++)
				{
					if (client_ptr->sock == clients[i].sock)
					{
						place = i;
						snprintf(message, BUF_SIZE, "%s disconnected !", client_ptr->name);

						send_message_to_all_clients(clients, *client_ptr, actual, message, 1);
						remove_client(clients, place, &actual);
						closesocket(client_ptr->sock);
					}
				}
				pthread_mutex_unlock(&mutexClient);
				break;
			}

			for (int i = 0; i < nbDefi; i++)
			{

				if (defis[i].clientQuiDefie->sock == client_ptr->sock && defis[i].status == 2)
				{
					while (defis[i].status == 2)
					{
						pthread_yield();
					}
				}
			}
			pthread_mutex_lock(&mutexDefi);
			for (int i = 0; i < nbDefi; i++)
			{
				if (defis[i].clientDefie->sock == client_ptr->sock && defis[i].status == 1)
				{
					recepDefi = 1;
					meDefi = &defis[i];
				}
			}
			pthread_mutex_unlock(&mutexDefi);
			if (recepDefi == 1)
			{
				if (streaming == -1)
				{
					streaming = 0;
					int pos = -1;
					snprintf(message, BUF_SIZE, "%s arrete de regarder", client_ptr->name);
					Defi *defiStream = &defis[choix];
					stream(defiStream, message);
					for (int i = 0; i < defis[choix].nbViewers; i++)
					{
						if (defis[choix].viewers[i].sock == client_ptr->sock)
						{
							pos = i;
						}
					}
					supprimerElement(defis[choix].viewers, &defis[choix].nbViewers, pos);
					streaming = 0;
					choix = -1;
				}
				if (enChat == 1)
				{
					int pos = -1;
					snprintf(message, BUF_SIZE, "%s quitte le chat", client_ptr->name);
					chatWrite(message);
					pthread_mutex_lock(&mutexChat);
					for (int i = 0; i < nbChat; i++)
					{
						if (chat[i].sock == client_ptr->sock)
						{
							pos = i;
						}
						pthread_mutex_unlock(&mutexChat);
					}
					pthread_mutex_lock(&mutexChat);
					supprimerElement(chat, &nbChat, pos);
					enChat = 0;
					pthread_mutex_unlock(&mutexChat);
				}
				lancerdefi = 0;
				if (strcmp(buffer, "1") == 0 && meDefi->status != 3)
				{
					meDefi->status = 2;
					commencer_partie(meDefi->clientDefie, meDefi->clientQuiDefie);
					meDefi->status = 0;
					recepDefi = 0;
				}
				else if (strcmp(buffer, "2") == 0 && meDefi->status != 3)
				{
					strncpy(message, client_ptr->name, BUF_SIZE - 1);
					strncat(message, " a refusé.e\n", BUF_SIZE - strlen(message) - 1);
					write_client(meDefi->clientQuiDefie->sock, message);
					meDefi->status = 3;
					recepDefi = 0;
				}
				else if (meDefi->status != 3)
				{
					// Message d'erreur si l'entrée n'est ni "1" ni "2"
					strncpy(message, "Vous devez choisir 1 ou 2", BUF_SIZE - 1);
					write_client(client_ptr->sock, message);
				}
				else if (meDefi->status == 3)
				{
					strncpy(message, "Votre adversaire s'est déconnecter tappez menu", BUF_SIZE - 1);
					write_client(client_ptr->sock, message);
					recepDefi = 0;
					lancerdefi = 0;
					meDefi->status = 3;
				}
			}
			else if (enChat == 1 && strcmp(buffer, "menu") != 0)
			{
				snprintf(message, BUF_SIZE, "%s : %s", client_ptr->name, buffer);
				chatWrite(message);
			}
			else if (strcmp(buffer, "menu") == 0 && enChat == 1)
			{
				int pos = -1;
				snprintf(message, BUF_SIZE, "%s quitte le chat", client_ptr->name);
				chatWrite(message);

				for (int i = 0; i < nbChat; i++)
				{
					if (chat[i].sock == client_ptr->sock)
					{
						pos = i;
					}
				}
				pthread_mutex_lock(&mutexChat);
				supprimerElement(chat, &nbChat, pos);
				enChat = 0;
				pthread_mutex_unlock(&mutexChat);
			}
			else if (streaming == 1)
			{
				choix = atoi(buffer);
				if (choix < nbDefi)
				{
					defis[choix].viewers[defis[choix].nbViewers] = *client_ptr;
					defis[choix].nbViewers++;
					snprintf(message, BUF_SIZE, "%s commence à regarder...", client_ptr->name);
					Defi *defiStream = &defis[choix];
					stream(defiStream, message);
					snprintf(message, BUF_SIZE, "Vous êtes dans la partie, vous pouvez regarder et ecrire dans le chat de la partie tappez menu pour quitter");
					write_client(client_ptr->sock, message);
					streaming = -1;
				}
			}
			else if (strcmp(buffer, "menu") == 0 && streaming == -1)
			{
				int pos = -1;
				snprintf(message, BUF_SIZE, "%s arrete de regarder", client_ptr->name);
				Defi *defiStream = &defis[choix];
				stream(defiStream, message);
				for (int i = 0; i < defis[choix].nbViewers; i++)
				{
					if (defis[choix].viewers[i].sock == client_ptr->sock)
					{
						pos = i;
					}
				}
				supprimerElement(defis[choix].viewers, &defis[choix].nbViewers, pos);
				streaming = 0;
				choix = -1;
			}
			else if (strcmp(buffer, "menu") != 0 && streaming == -1)
			{
				snprintf(message, BUF_SIZE, "%s : %s", client_ptr->name, buffer);
				Defi *defiStream = &defis[choix];
				stream(defiStream, message);
			}
			else if (lancerdefi == 1 && recepDefi == 0 && strcmp(buffer, "menu") != 0)
			{
				choix = atoi(buffer);
				if (choix < actual && clients[choix].dispo == 1)
				{

					snprintf(message, BUF_SIZE, "%s vous défie! 1 pour accepter et  2 pour refuser", client_ptr->name);
					pthread_mutex_lock(&mutexClient);
					write_client(clients[choix].sock, message);
					Defi defi;
					defi.clientDefie = &clients[choix];
					pthread_mutex_unlock(&mutexClient);
					defi.clientQuiDefie = client_ptr;
					defi.status = 1;
					pthread_mutex_lock(&mutexDefi);
					defis[nbDefi] = defi;
					nbDefi++;
					pthread_mutex_unlock(&mutexDefi);
					lancerdefi = 0;
				}
				else
				{
					snprintf(message, BUF_SIZE, "Ce joueur n'est pas disponible\nVeuillez en choisir un autre ou revenir au menu  ");
					write_client(client_ptr->sock, message);
				}
				choix = -1;
			}
			else if ((lancerdefi == 0 && recepDefi == 0 && (strcmp(buffer, "1") != 0 && strcmp(buffer, "2") != 0 && strcmp(buffer, "3") != 0 && strcmp(buffer, "0") != 0 && strcmp(buffer, "4") != 0)) || strcmp(buffer, "menu") == 0)
			{
				recepDefi = 0;
				streaming = 0;
				enChat = 0;
				lancerdefi = 0;
				client_ptr->dispo = 1;
				snprintf(message, BUF_SIZE, "BOUJOUR %s\nTappe 1 si tu veux jouer, tappe 2 si tu veux regarder une partie, tappe 3 pour voir l'historique des parties, tappe 4 pour accéder au chat général", client_ptr->name);
				write_client(client_ptr->sock, message);
			}

			else if (strcmp(buffer, "1") == 0 && recepDefi == 0 && lancerdefi == 0)
			{
				// snprintf(message, BUF_SIZE, "cacacaaaaa");
				// write_client(client_ptr->sock, message);
				client_ptr->dispo = 1;
				pthread_mutex_lock(&mutexClient);
				send_connected_clients_list(clients, actual, client_ptr->sock);
				pthread_mutex_unlock(&mutexClient);
				snprintf(message, BUF_SIZE, "Choisis ton adversaire !\nTappe 'menu' pour revenir au menu");
				write_client(client_ptr->sock, message);
				lancerdefi = 1;
			}
			else if (strcmp(buffer, "2") == 0 && recepDefi == 0 && lancerdefi == 0 && streaming == 0)
			{
				// Lister toutes les défis en cours avec les joueurs à l'intérieur
				char message[BUF_SIZE];
				int nbDefiEnCours = 0;
				pthread_mutex_lock(&mutexDefi);
				for (int i = 0; i < nbDefi; i++)
				{
					if (defis[i].status == 2)
					{
						nbDefiEnCours++;
					}
				}
				if (nbDefiEnCours == 0)
				{
					snprintf(message, BUF_SIZE, "Il n'y a pas de défis en cours pour le moment. \nTappe 'menu' pour revenir au menu");
					write_client(client_ptr->sock, message);
				}
				else
				{
					snprintf(message, BUF_SIZE, "Liste des défis en cours :\n");
					write_client(client_ptr->sock, message);
					for (int i = 0; i < nbDefi; i++)
					{
						if (defis[i].status == 2)
						{
							snprintf(message, BUF_SIZE, "%d) Défi en cours entre %s et %s  Score : %d - %d \n", i, defis[i].clientDefie->name, defis[i].clientQuiDefie->name, defis[i].partie->points[0], defis[i].partie->points[1]);
							write_client(client_ptr->sock, message);
						}
						snprintf(message, BUF_SIZE, "Tappe le numéro de la partie que tu veux streamer ou tappe 'menu' pour revenir au menu");
						write_client(client_ptr->sock, message);
					}
					streaming = 1;
				}
				pthread_mutex_unlock(&mutexDefi);
			}
			else if (strcmp(buffer, "3") == 0 && recepDefi == 0 && lancerdefi == 0)
			{
				// Lister toutes les défis en cours avec les joueurs à l'intérieur
				char message[BUF_SIZE];
				pthread_mutex_lock(&mutexDefi);
				if (nbDefi == 0)
				{
					snprintf(message, BUF_SIZE, "Il n'y a pas encore eu de défis pour le moment. \nTappe 'menu' pour revenir au menu");
					write_client(client_ptr->sock, message);
				}
				else
				{
					snprintf(message, BUF_SIZE, "Historique des défis :\n");
					write_client(client_ptr->sock, message);
					for (int i = 0; i < nbDefi; i++)
					{
						char statut[20];
						if (defis[i].status == 0)
						{
							strcpy(statut, "partie finie");
						}
						else if (defis[i].status == 1)
						{
							strcpy(statut, "en lancement");
						}
						else if (defis[i].status == 2)
						{
							strcpy(statut, "en cours");
						}
						else if (defis[i].status == 3)
						{
							strcpy(statut, "interrompue");
						}

						snprintf(message, BUF_SIZE, "- Défi entre %s et %s - Statut : %s\n", defis[i].clientDefie->name, defis[i].clientQuiDefie->name, statut);
						write_client(client_ptr->sock, message);

						if (defis[i].status == 0 || defis[i].status == 2)
						{
							snprintf(message, BUF_SIZE, "Score : %d - %d\n", defis[i].partie->points[0], defis[i].partie->points[1]);
							write_client(client_ptr->sock, message);
						}
					}
					snprintf(message, BUF_SIZE, "Tappe 'menu' pour revenir au menu");
					write_client(client_ptr->sock, message);
				}
				pthread_mutex_unlock(&mutexDefi);
			}
			else if (strcmp(buffer, "4") == 0 && recepDefi == 0 && lancerdefi == 0 && streaming == 0)
			{
				pthread_mutex_lock(&mutexChat);
				chat[nbChat] = *client_ptr;
				nbChat++;
				snprintf(message, BUF_SIZE, "%s rejoins le chat...", client_ptr->name);
				chatWrite(message);
				enChat = 1;
				pthread_mutex_unlock(&mutexChat);
			}
		}
	}
	pthread_exit(NULL);
}

static void commencer_partie(Client *joueur1, Client *joueur2)
{
	joueur1->dispo = 0;
	joueur2->dispo = 0;
	char buffer[BUF_SIZE];
	char mess[BUF_SIZE];
	Partie partie;
	Partie *partiePtr = &partie;
	Defi *ceDefi;
	pthread_mutex_lock(&mutexDefi);
	for (int i = 0; i < nbDefi; i++)
	{
		if (((defis[i].clientQuiDefie->sock == joueur1->sock && defis[i].clientDefie->sock == joueur2->sock) || (defis[i].clientQuiDefie->sock == joueur2->sock && defis[i].clientDefie->sock == joueur1->sock)) && defis[i].status == 2)
		{
			defis[i].partie = &partie;
			ceDefi = &defis[i];
			ceDefi->nbViewers = 2;
			ceDefi->viewers[0] = *joueur1;
			ceDefi->viewers[1] = *joueur2;
		}
	}
	pthread_mutex_unlock(&mutexDefi);

	srand(time(NULL));
	int randomValue = rand() % 2;
	Client *actu = (randomValue == 0) ? joueur1 : joueur2;

	// Envoyer au deux qui commence
	snprintf(buffer, BUF_SIZE, "%s commence\n", actu->name);
	stream(ceDefi, buffer);
	// Le joueur qui commence la partie choisi le sens
	int sens = 0;
	snprintf(buffer, BUF_SIZE, "Dans quel sens veux-tu jouer ?\n1) Horaire\n2) Anti-Horaire?");
	write_client(actu->sock, buffer);
	read_client(actu->sock, buffer);

	sens = atoi(buffer);

	if (sens == 1)
	{
		snprintf(buffer, BUF_SIZE, "Le sens est horaire\n%s est en haut et %s est en bas\nA vous de jouer !", joueur1->name, joueur2->name);
	}
	else if (sens == 2)
	{
		snprintf(buffer, BUF_SIZE, "Le sens est antihoraire\n%s est en haut et %s est en bas\nA vous de jouer !", joueur1->name, joueur2->name);
	}

	stream(ceDefi, buffer);
	// init la partie
	startGame(partiePtr, randomValue, sens);

	char *plateau = afficher(partiePtr);

	stream(ceDefi, plateau);
	int gagnant;
	int c;

	while ((gagnant = finJeu(partiePtr)) == 0)
	{
		if (ceDefi->status == 3)
		{
			break;
		}
		int numCase;
		snprintf(buffer, BUF_SIZE, "Joueur %s : Quelle case (de 1 à 6 de gauche à droite)?", actu->name);
		write_client(actu->sock, buffer);

		int nourrir = checkNourrir(partiePtr);
		if (nourrir != -1)
		{
			snprintf(buffer, BUF_SIZE, "Il faut nourrir l'adversaire. Veuillez par exemple jouer le coup %d", nourrir - 1);
			write_client(actu->sock, buffer);
		}

		int isValidInput = 0;

		while (!isValidInput)
		{
			c = read_client(actu->sock, buffer);
			if (c == 0)
			{
				pthread_mutex_lock(&mutexDefi);
				for (int i = 0; i < nbDefi; i++)
				{
					if (defis[i].status == 1 || defis[i].status == 2)
					{
						if (defis[i].clientQuiDefie->sock == actu->sock || defis[i].clientDefie->sock == actu->sock)
						{
							// Trouvé un défi actif impliquant le joueur déconnecté
							defis[i].status = 3; // Statut interrompu
							snprintf(mess, BUF_SIZE, "Votre adversaire s'est déconnecté !! Tapez 'menu' pour revenir au menu");
							write_client((actu->sock == defis[i].clientQuiDefie->sock) ? defis[i].clientDefie->sock : defis[i].clientQuiDefie->sock, mess);
							break; // Vous pouvez sortir de la boucle après avoir trouvé le défi actif
						}
					}
				}
				pthread_mutex_unlock(&mutexDefi);
				int place;
				pthread_mutex_lock(&mutexClient);
				for (int i = 0; i < actual; i++)
				{
					if (actu->sock == clients[i].sock)
					{
						place = i;
						snprintf(mess, BUF_SIZE, "%s disconnected !", actu->name);

						// send_message_to_all_clients(clients, *actu, actual, mess, 1);
						closesocket(clients[i].sock);
						remove_client(clients, place, &actual);
					}
				}
				pthread_mutex_unlock(&mutexClient);
				break;
			}

			int isNumeric = 1;
			for (int i = 0; buffer[i] != '\0'; i++)
			{
				if (!isdigit(buffer[i]))
				{
					isNumeric = 0;
					break;
				}
			}
			if (isNumeric)
			{
				numCase = atoi(buffer);
				if (numCase >= 1 && numCase <= 6)
				{
					if ((partie.joueurActuel == 1 && partie.plateau[numCase - 1] == 0) || (partie.joueurActuel == 2 && partie.plateau[12 - numCase] == 0))
					{
						snprintf(buffer, BUF_SIZE, "La case est vide. Veuillez entrer un autre chiffre");
						write_client(actu->sock, buffer);
					}
					else
					{
						isValidInput = 1;
					}
				}
				else
				{
					snprintf(buffer, BUF_SIZE, "Saisie invalide. Assurez-vous que le chiffre est entre 1 et 6.");
					write_client(actu->sock, buffer);
				}
			}
			else
			{
				snprintf(buffer, BUF_SIZE, "Saisie invalide. Assurez-vous d'entrer un chiffre.");
				write_client(actu->sock, buffer);
			}
		}
		if (c == 0)
		{
			ceDefi->status = 3;
			break;
		}

		jouerCoup(partiePtr, numCase);
		plateau = afficher(partiePtr);

		stream(ceDefi, plateau);
		snprintf(buffer, BUF_SIZE, "Points : %s %d, %s %d\n", joueur1->name, partie.points[0], joueur2->name, partie.points[1]);

		stream(ceDefi, buffer);
		partie.joueurActuel = (partie.joueurActuel == 1) ? 2 : 1;
		actu = (actu == joueur2) ? joueur1 : joueur2;
	}
	if (ceDefi->status == 3)
	{
		fct_client(joueur1);
		joueur1->dispo = 1;
	}
	else
	{

		char message[500];
		if (gagnant == 1)
		{
			snprintf(message, sizeof(message), "\n\nFINI\n_______\n\nLe score est %d à %d et le gagnant est le joueur %s\n", partie.points[0], partie.points[1], joueur1->name);
		}
		else if (gagnant == 2)
		{
			snprintf(message, sizeof(message), "\n\nFINI\n_______\n\nLe score est %d à %d et le gagnant est le joueur %s\n", partie.points[0], partie.points[1], joueur2->name);
		}
		else
		{
			snprintf(message, sizeof(message), "\n\nFINI\n_______\n\nLe score est %d à %d et il y a égalité !   :)\n", partie.points[0], partie.points[1]);
		}

		stream(ceDefi, message);
		joueur1->dispo = 1;
		joueur2->dispo = 1;
	}
}

void stream(Defi *defi, char *message)
{
	for (int i = 0; i < defi->nbViewers; i++)
	{
		write_client(defi->viewers[i].sock, message);
	}
}

void chatWrite(char *message)
{
	for (int i = 0; i < nbChat; i++)
	{
		write_client(chat[i].sock, message);
	}
}

static void clear_clients(Client *clients, int actual)
{
	int i = 0;
	for (i = 0; i < actual; i++)
	{
		closesocket(clients[i].sock);
	}
}

static void remove_client(Client *clients, int to_remove, int *actual)
{
	/* we remove the client in the array */
	memmove(clients + to_remove, clients + to_remove + 1, (*actual - to_remove - 1) * sizeof(Client));
	/* number client - 1 */
	(*actual)--;
}

void supprimerElement(Client tableau[], int *taille, int indice)
{
	if (indice < 0 || indice >= *taille)
	{
		printf("Indice invalide.\n");
		return;
	}

	for (int i = indice; i < (*taille - 1); i++)
	{
		tableau[i] = tableau[i + 1];
	}

	(*taille)--;
}

static void send_message_to_all_clients(Client *clients, Client sender, int actual, const char *buffer, char from_server)
{

	char message[BUF_SIZE];
	// printf("CACA :\n");
	snprintf(message, BUF_SIZE, "%s", buffer);
	for (int i = 0; i < actual; i++)
	{

		if (sender.sock != clients[i].sock)
		{
			if (from_server == 0)
			{
				strncpy(message, sender.name, BUF_SIZE - 1);
				strncat(message, " : ", sizeof message - strlen(message) - 1);
			}
			write_client(clients[i].sock, message);
		}
	}
}

static void send_connected_clients_list(Client *clients, int actual, int currentClientSocket)
{
	char message[BUF_SIZE];
	strcpy(message, "Pseudos en ligne : \n");
	for (int i = 0; i < actual; i++)
	{
		char clientInfo[BUF_SIZE];
		snprintf(clientInfo, BUF_SIZE, "%d) %s %s", i, clients[i].name, (clients[i].dispo == 1) ? "(dispo)" : "(pas dispo)");

		strcat(clientInfo, "\n");

		strcat(message, clientInfo);
	}

	// Send the list only to the current client
	write_client(currentClientSocket, message);
}

static int init_connection(void)
{
	SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
	SOCKADDR_IN sin = {0};

	if (sock == INVALID_SOCKET)
	{
		perror("socket()");
		exit(errno);
	}

	sin.sin_addr.s_addr = htonl(INADDR_ANY);
	sin.sin_port = htons(PORT);
	sin.sin_family = AF_INET;

	if (bind(sock, (SOCKADDR *)&sin, sizeof sin) == SOCKET_ERROR)
	{
		perror("bind()");
		exit(errno);
	}

	if (listen(sock, MAX_CLIENTS) == SOCKET_ERROR)
	{
		perror("listen()");
		exit(errno);
	}

	return sock;
}

static void end_connection(int sock)
{
	closesocket(sock);
}

static int read_client(SOCKET sock, char *buffer)
{
	int n = 0;

	if ((n = recv(sock, buffer, BUF_SIZE - 1, 0)) < 0)
	{
		perror("recv()");
		saveJSON();
		/* if recv error we disonnect the client */
		n = 0;
	}

	buffer[n] = 0;

	return n;
}

static void write_client(SOCKET sock, const char *buffer)
{
	if (send(sock, buffer, strlen(buffer), 0) < 0)
	{
		perror("send()");
		saveJSON();
		exit(errno);
	}
}

int main(int argc, char **argv)
{
	init();

	app();

	end();

	return EXIT_SUCCESS;
}
