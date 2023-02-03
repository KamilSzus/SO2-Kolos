
#include <malloc.h>
#include <stdint.h>
#include <semaphore.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>

struct shm{
    int32_t array[100];
    int32_t poszukiwanaWartos;
    int32_t pierwszeWystapienie;
    int32_t iloscWystapien;
    sem_t client;
    sem_t server;
};

int *read_numbers(char *filename, int *size);

int main(int argc,char** argv){

    sem_t *sem = sem_open("/msg_signal", 0);

    int temp;

    sem_getvalue(sem,&temp);

    printf("\n%d\n",temp);

    if (temp <= 0){
        printf("\nSerwer zajety\n");

        return 0;
    }

    sem_wait(sem);

    FILE *file;

    file = fopen(argv[1], "r");
    if(!file){

        return -2;
    }


    printf("\nZajmuje serwer\n");

    int fd = shm_open("/msg_data", O_RDWR, 0600);

    struct shm *pdata = (struct shm *) mmap(NULL, sizeof(struct shm), PROT_READ | PROT_WRITE, MAP_SHARED, fd,
                                            0);
    pdata->iloscWystapien = 0;
    pdata->poszukiwanaWartos = atoi(argv[2]);
    pdata->pierwszeWystapienie = 0;
    sleep(20);

    int i = 0;
    while (!feof(file)) {
        fscanf(file, "%d",&pdata->array[i]);
        i++;
        if (i>100){
            break;
        }
    }

    sem_post(&pdata->client);

    printf("\nWysylam dane do serwera: \n");

    sem_wait(&pdata->server);


    printf("Otrzymane dane "
           "\n liczba poszukiwana %d"
           "\nindeks pierwszego wystapienia %d"
           "\n ilosc wystapien %d", pdata->poszukiwanaWartos,pdata->pierwszeWystapienie,pdata->iloscWystapien);

    sem_post(&pdata->client);

    munmap(pdata, sizeof(struct shm));
    close(fd);
    fclose(file);

    return 0;
}

int *read_numbers(char *filename, int *size) {
    FILE *file;

    file = fopen(filename, "r");

    int numbers_size = 10;
    int *numbers = calloc(numbers_size, sizeof(int));
    int i = 0;
    int *new_numbers = NULL;

    if (numbers == NULL)
        return NULL;

    while (!feof(file))
    {
        fscanf(file, "%d", &numbers[i]);
        printf("%d ", numbers[i]);
        i++;
        if (numbers_size <= i) {
            numbers_size *= 2;
            new_numbers = realloc(numbers, numbers_size * sizeof(int));
            if (new_numbers == NULL) {
                free(numbers);
                fclose(file);
                return NULL;
            }
            numbers = new_numbers;
        }
    }

    new_numbers = realloc(numbers, i * sizeof(int));
    if (new_numbers == NULL) {
        free(numbers);
    }

    *size = i;

    fclose(file);

    return new_numbers;
}