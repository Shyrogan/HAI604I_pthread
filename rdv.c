/**
 * Copyright 2023 <Sébastien>
 */
#include "./calcul.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

struct TasksCommon {
  int workTime;
  int count;

  int finished;

  pthread_mutex_t mutex;
  pthread_cond_t cond;
};

struct Task {
  int id;
  pthread_t thread;

  struct TasksCommon *common;
};

void *handle_task(void *v_params) {
  struct Task *task = (struct Task *)v_params;
  printf("[Thread %i] Départ !\n", task->id);

  // On fait notre travail
  calcul(task->common->workTime);

  if (pthread_mutex_lock(&(task->common->mutex)), NULL) {
    printf("Erreur lors du vérouillage du mutex...\n");
    exit(1);
  }
  task->common->finished++;
  printf("[Thread %i] Finito pipo.\n", task->id);

  if (task->common->finished < task->common->count) {
    if (pthread_cond_wait(&(task->common->cond), &(task->common->mutex))) {
      printf("Erreur lors de l'attente du cond...\n");
      exit(1);
    }
  } else {
    pthread_cond_broadcast(&(task->common->cond));
    printf("[Thread %i] Tout le monde a fini apparemment! Retournons "
           "travailler un peu...\n",
           task->id);
  }
  pthread_mutex_unlock(&(task->common->mutex));

  printf("[Thread %i] Je retravaille (environ %is)\n", task->id,
         task->common->workTime * 3);
  calcul(task->common->workTime);
  printf("[Thread %i] Adieu!\n", task->id);

  pthread_exit(0);
}

int main(int argc, char *argv[]) {
  if (argc != 3) {
    printf("Utilisation: %s <nb de threads> <temps de travail en sec>\n",
           argv[0]);
    exit(0);
  }

  struct TasksCommon common;
  common.count = atoi(argv[1]);
  common.workTime = atoi(argv[2]);
  common.finished = 0;

  if (pthread_mutex_init(&(common.mutex), NULL)) {
    printf("Erreur lors de la création du mutex partagé.\n");
    exit(1);
  }

  if (pthread_cond_init(&(common.cond), NULL)) {
    printf("Erreur lors de la création du cond partagé.\n");
    exit(1);
  }

  printf("Démarrage des %i tâches...\n", common.count);
  struct Task *tasks = malloc(sizeof(struct Task) * common.count);
  for (int i = 0; i < common.count; i++) {
    struct Task *task = &(tasks[i]);
    task->id = i + 1;
    task->common = &common;

    if (pthread_create(&(task->thread), NULL, handle_task, task)) {
      printf("Erreur lors de la création du thread %i\n", task->id);
      exit(1);
    }
  }

  for (int i = 0; i < common.count; i++) {
    pthread_join(tasks[i].thread, NULL);
  }

  free(tasks);

  return 0;
}
