#include <stdio.h>
#include <pthread.h>
#include <stdint.h>
#include <semaphore.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>


struct serverStruct{
    int clientNumber;
    char quit[1024];
    sem_t *exit;
    pthread_mutex_t mutex;
};

struct shm{
    int32_t array[100];
    int32_t poszukiwanaWartos;
    int32_t pierwszeWystapienie;
    int32_t iloscWystapien;
    sem_t client;
    sem_t server;
};

sem_t *sem;

void *clientJob(void *data) {
    struct shm *pdata = (struct shm *) data;
    int liczbaSiePojawila = 0;
    for (int i = 0; i < 100; ++i) {
        if (pdata->array[i] == pdata->poszukiwanaWartos && liczbaSiePojawila == 0) {
            pdata->pierwszeWystapienie = i;
            //indeks od zera czy od 1?
            liczbaSiePojawila = 1;
        }

        if (pdata->array[i] == pdata->poszukiwanaWartos) {
            pdata->iloscWystapien++;
        }

    }
    printf("\nKoniec procesowania danych \n");

    return 0;
}

void *serverExit(void *quit) {
    struct serverStruct *exit = (struct serverStruct *) quit;

    while (strcmp(exit->quit, "quit") != 0) {
        printf("wpisz 'quit' aby zamknac serwer lub 'stat' dla statystyk");
        char *p = fgets(exit->quit, 1024, stdin);
        pthread_mutex_lock(&exit->mutex);
        if (!p) break;
        if (*exit->quit) {
            exit->quit[strlen(exit->quit) - 1] = '\0';
        }

        if (strcmp(exit->quit, "stat") == 0 ) {
            printf("\nstatystyki serwera %d\n", exit->clientNumber);
        }
        pthread_mutex_unlock(&exit->mutex);
    }
    sem_post(exit->exit);

    return 0;
}

int main() {
    sem = sem_open("/msg_signal", O_CREAT, 0600, 0);
    int fd = shm_open("/msg_data", O_CREAT | O_RDWR, 0600);

    ftruncate(fd, sizeof(struct shm));
    struct shm *pdata = (struct shm *) mmap(NULL, sizeof(struct shm), PROT_READ | PROT_WRITE, MAP_SHARED, fd,
                                            0);
    sem_init(&pdata->client, 1, 0);
    sem_init(&pdata->server, 1, 0);
    printf("Gotowe, czekam na klienta\n");

    struct serverStruct* exitAndStat = calloc(1, sizeof(struct serverStruct));
    exitAndStat->exit = &pdata->client;
    pthread_mutex_init(&exitAndStat->mutex,NULL);

    pthread_t clientThread;
    pthread_t statAndQuitThread;

    pthread_create(&statAndQuitThread, NULL, serverExit, exitAndStat);

    while (strcmp(exitAndStat->quit, "quit") != 0) {
        sem_post(sem);
        sem_wait(&pdata->client);

        if (strcmp(exitAndStat->quit, "quit") == 0){
            break;
        }

        printf("\ndostałem dane od klienta\n");
        pthread_create(&clientThread, NULL, clientJob, pdata);
        pthread_join(clientThread, 0);

        sem_post(&pdata->server);
        sem_wait(&pdata->client);

        pthread_mutex_lock(&exitAndStat->mutex);
        exitAndStat->clientNumber++;
        pthread_mutex_unlock(&exitAndStat->mutex);

        printf("\nCzekam na nowego klienta\n");
    }


    pthread_join(statAndQuitThread, 0);
    pthread_mutex_destroy(&exitAndStat->mutex);
    free(exitAndStat);
    munmap(pdata, sizeof(struct shm));
    close(fd);
    shm_unlink("/msg_data");
    sem_close(sem);
    sem_unlink("/msg_signal");

    return 0;
}
