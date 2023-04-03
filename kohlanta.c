/**
 * Copyright 2023 <Sébastien>
 */
#include "./calcul.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <threads.h>

struct Shared {
  float bassineSize;
  int teamCount;
  int playerPerTeamCount;
  int travelTime;
  int randomMarginTravelTime;
  float waterCollected;
  float waterLost;

  struct Team *teams;
};

struct Team {
  int id;
  pthread_t thread;
  struct Shared *shared;

  // Initialized by team
  struct Player *players;

  // Mutable data
  pthread_mutex_t mutex;
  pthread_cond_t cond;
  float bassine;
};

struct Player {
  int id;
  pthread_t thread;
  struct Team *team;
};

void *handle_player(void *v_params) {
  struct Player *player = (struct Player *)v_params;

  while (player->team->bassine < player->team->shared->bassineSize) {
    if (pthread_cond_wait(&(player->team->cond), &(player->team->mutex))) {
      printf("[Team %i, player %i] Erreur lors de l'attente du cond.\n",
             player->team->id, player->id);
      exit(EXIT_FAILURE);
    }
    if (player->team->bassine >= player->team->shared->bassineSize) {
      break;
    }

    // Comme ça les deux peuvent partir.
    pthread_mutex_unlock(&(player->team->mutex));

    // On part faire un aller, on peut aller un peu plus vite ou un peu moins
    // vite que le temps moyen indiqué.
    printf("[Team %i, player %i] Je pars!\n", player->team->id, player->id);
    // RAND_MAX vaut 32767 par défaut, la divison nous permet d'avoir un nombre
    // entre 0 et 1
    int tempsArRand = (rand() % player->team->shared->randomMarginTravelTime) -
                      player->team->shared->randomMarginTravelTime / 2;
    calcul(player->team->shared->travelTime + tempsArRand);

    if (pthread_mutex_lock(&(player->team->mutex))) {
      printf("[Team %i, player %i] Erreur lors du lock du mutex.\n",
             player->team->id, player->id);
      exit(EXIT_FAILURE);
    }
    float eauRand =
        ((rand() / (float)RAND_MAX) * player->team->shared->waterLost) -
        player->team->shared->waterLost / 2.0;
    printf("[Team %i, player %i] Je prends de l'eau (%0.2fL pris, je vais en "
           "perdre %fL) et je repars!\n",
           player->team->id, player->id, player->team->shared->waterCollected,
           eauRand);
    pthread_mutex_unlock(&(player->team->mutex));
    calcul(player->team->shared->travelTime + tempsArRand);
    if (pthread_mutex_lock(&(player->team->mutex))) {
      printf("[Team %i, player %i] Erreur lors du lock du mutex.\n",
             player->team->id, player->id);
      exit(EXIT_FAILURE);
    }
    player->team->bassine += player->team->shared->waterCollected + eauRand;
    printf("[Team %i, player %i] Au suivant (%0.2fL/%0.2f)!\n",
           player->team->id, player->id, player->team->bassine,
           player->team->shared->bassineSize);
    pthread_mutex_unlock(&(player->team->mutex));
    pthread_cond_signal(&(player->team->cond));
  }

  printf("[Team %i, player %i] Mon équipe a gagné!\n", player->team->id,
         player->id);

  free(player->team->players);
  free(player->team->shared->teams);
  // Faudrait faire pareil avec les mutex mais flemme là

  exit(0);
}

void *handle_team(void *v_params) {
  struct Team *team = (struct Team *)v_params;

  printf("[Team %i] Initialisation de l'équipe. (Joueurs: %i, taille "
         "bassine: %0.2f)\n",
         team->id, team->shared->playerPerTeamCount, team->bassine);

  team->players =
      malloc(sizeof(struct Player) * team->shared->playerPerTeamCount);
  for (int p = 0; p < team->shared->playerPerTeamCount; p++) {
    struct Player *player = &(team->players[p]);
    player->id = p + 1;
    player->team = team;

    if (pthread_create(&(player->thread), NULL, handle_player, player)) {
      printf("[Team %i, player %i] Erreur lors de l'initialisation du thread.",
             team->id, player->id);
    }
  }
  pthread_cond_signal(&(team->cond));
  pthread_cond_signal(&(team->cond));

  for (int p = 0; p < team->shared->playerPerTeamCount; p++) {
    pthread_join(team->players[p].thread, NULL);
  }

  free(team->players);
  pthread_exit(NULL);
}

int main(int argc, char *argv[]) {
  if (argc != 8) {
    printf("Usage: %s <nb équipes> <nb joueurs par équipe> "
           "<taille bassine> <temps aller (et retour)> <marge aléatoire temps "
           "a/r> <eau par a/r> <perte par a/r>\n",
           argv[0]);
    exit(EXIT_SUCCESS);
  }

  srand(time(NULL));

  // Ouais c'est très flexible
  struct Shared shared;
  shared.teamCount = atoi(argv[1]);
  shared.playerPerTeamCount = atoi(argv[2]);
  shared.bassineSize = atof(argv[3]);
  shared.teams = malloc(sizeof(struct Team) * shared.teamCount);
  shared.travelTime = atoi(argv[4]);
  shared.randomMarginTravelTime = atoi(argv[5]);
  shared.waterCollected = atof(argv[6]);
  shared.waterLost = atof(argv[7]);

  for (int t = 0; t < shared.teamCount; t++) {
    struct Team *team = &(shared.teams[t]);
    team->id = t + 1;
    team->bassine = 0;
    team->shared = &shared;

    if (pthread_cond_init(&(team->cond), NULL)) {
      printf("[Team %i] Erreur lors de l'initialisation du cond.\n", team->id);
      exit(EXIT_FAILURE);
    }

    if (pthread_mutex_init(&(team->mutex), NULL)) {
      printf("[Team %i] Erreur lors de l'initialisation du mutex.\n", team->id);
      exit(EXIT_FAILURE);
    }

    if (pthread_create(&(team->thread), NULL, handle_team, team)) {
      printf("[Team %i] Erreur lors de l'initialisation du thread.\n",
             team->id);
      exit(EXIT_FAILURE);
    }
  }

  for (int t = 0; t < shared.teamCount; t++) {
    pthread_join(shared.teams[t].thread, NULL);
  }

  free(shared.teams);

  return EXIT_SUCCESS;
}
