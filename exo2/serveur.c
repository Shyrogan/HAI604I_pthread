#include <netinet/in.h>
#include <stdio.h>//perror
#include <sys/types.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>//close
#include <stdlib.h>
#include <string.h>

#define MAX_BUFFER_SIZE 146980


/* ========================= Multithreading =========================
Le but est de creer un serveur qui va :
- Creer SA propre socket
- Ecouter si un client arrive
- Si un client arrive, creer thread qui le gere avec un socket de session
  (qui ne dure que pendant l'echange)
- Boucle infinie

Pourquoi multithreading ? Psk dans la boucle du while, on deplace le code
pour que tout le travail soit fait par un thread.
La boucle while qui suit le "listen" devient une boucle qui creee des threads

Attention, on utilise "detach" pour ne pas attendre la fin du thread,
et surtout, on ne FERME PAS la socket de SESSION, sinon il plante, donc on
"continue" l'execution, et la socket de SESSION est fermee par le thread

Attention, pour ne pas avoir de conflits d'ecriture, chaque thread
rajoute son numero au nom du fichier
*/
#include <pthread.h>

struct paramsThread {
  int idThread;
  int sessionfd;
};

void *thread_function (void *params) {

  struct paramsThread *args = (struct paramsThread *) params;

  printf("[%d] Thread starts work\n", args -> idThread);
  
  // ========================= DEBUT CODE RESEAU =======================================
  int filename_size, file_size;
  char filename[filename_size];
  
  // FILE NAME SIZE
  recv(args -> sessionfd, &filename_size, sizeof(filename_size), 0) < 0
    ? perror("recv() filename size"), exit(1)
    : printf("[%d] -- File name size: %d\n", args -> idThread, filename_size);

  // FILE NAME
  memset(&filename, 0, sizeof(filename));
  recv(args -> sessionfd, filename, filename_size, 0) < 0
    ? perror("recv() file name"), exit(1)
    : printf("[%d] -- File name: %s\n", args -> idThread, filename);

  // FILE SIZE
  recv(args -> sessionfd, &file_size, sizeof(file_size), 0) < 0
    ? perror("recv() file size"), exit(1)
    : printf("[%d] -- File size: %d octets\n", args -> idThread, file_size);

  // PREPARE RECEIVING FILE
  char filepath[256] = "./reception/"; // cette ligne n'est bien-sur qu'un exemple et doit être modifiée : le nom du fichier doit être reçu.

  // FORMAT : ./reception/[idThread]filename
  char num_thread[5];
  sprintf(num_thread, "%d_", args -> idThread);
  strcat(filepath, num_thread);
  strcat(filepath, filename);

  FILE* file = fopen(filepath, "wb");
  if(file == NULL){
    perror("fopen()\n"); exit(1);
  }

  int total_received = 0;

  while(total_received < file_size) {

    // WRITE TO FILE
    char buffer[MAX_BUFFER_SIZE];

    int block = recv(args -> sessionfd, buffer, MAX_BUFFER_SIZE, 0);
    block < 0
      ? perror("recv() file data"), exit(1)
      : printf("[%d] Received block of %d octet\n", args -> idThread, block);

    total_received += block;

    size_t written = fwrite(buffer, sizeof(char), block, file);
    if (written < strlen(buffer)+1) {
      perror("fwrite()"); fclose(file); exit(1);
    } else {
      printf("[%d] Wrote %zd octets to file\n", args -> idThread, written);
    }
  }

  printf("[%d] Connection closed (total %d octets)\n", args -> idThread, total_received);


  fclose(file);
  close(args -> sessionfd);
  // ========================= FIN CODE RESEAU =======================================

  printf("[%d] Thread Exiting...\n\n", args -> idThread);
}




int main(int argc, char *argv[])
{
  /* etape 0 : gestion des paramètres si vous souhaitez en passer */

  /* etape 1 : creer une socket d'écoute des demandes de connexions*/
  int sockfd = socket(PF_INET, SOCK_STREAM, 0);
  sockfd <= 0
    ? perror("socket()"), exit(1)
    : printf("[-] Socket created\n");

  /* etape 2 : nommage de la socket */
  socklen_t len = sizeof(struct sockaddr_in);
  struct sockaddr_in me, client;
  me.sin_family = AF_INET;
  me.sin_addr.s_addr = htonl(INADDR_ANY);
  me.sin_port = htons(4444);

  bind(sockfd, (struct sockaddr*) &me, len) < 0
    ? perror("bind()"), exit(1)
    : printf("[-] Socket binded\n");

  getsockname(sockfd, (struct sockaddr*) &me, &len) < 0
    ? perror("getsockname()"), exit(1)
    : printf("[-] Port Assigned to %d\n", ntohs(me.sin_port));

  /* etape 3 : mise en ecoute des demandes de connexions */
  listen(sockfd, 30) < 0
    ? perror("listen()"), exit(1)
    : printf("[-] Server listening...\n");

  /* etape 4 : plus qu'a attendre la demadne d'un client */

  int i = 1;
  while(1) {
    int sessionfd = accept(sockfd, (struct sockaddr*) &client, &len);
    printf("[-] Client %s:%d connected\n", inet_ntoa(client.sin_addr), htons(client.sin_port));

    pthread_t thread;
    struct paramsThread *parameters = malloc(sizeof(struct paramsThread));
    parameters -> idThread = i++;
    parameters -> sessionfd = sessionfd;

    if (pthread_create(&thread, NULL, &thread_function, parameters) != 0) {
      perror("pthread_create");
      close(sessionfd);
      continue;
    }

    if (pthread_detach(thread) != 0) {
      perror("pthread_join()"); exit(1);
    }

  }

  close(sockfd); printf("[-] Server exiting\n");
}