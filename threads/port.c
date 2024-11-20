#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

#define DOCKS 4
#define TUGS 12


pthread_mutex_t docks_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t tugs_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t tugs_available = PTHREAD_COND_INITIALIZER, dock_available = PTHREAD_COND_INITIALIZER;

int free_docks = DOCKS;
int free_tugs = TUGS;

void *ship_thread(void *m) {
    int mass = *((int *) m);
    pthread_mutex_lock(&docks_mutex);
    if (free_docks == 0) {
        pthread_cond_wait(&dock_available, &docks_mutex);
    }
    free_docks--;
    pthread_mutex_unlock(&docks_mutex);

    pthread_mutex_lock(&tugs_mutex);
    while (free_tugs < mass) {
        pthread_cond_wait(&tugs_available, &tugs_mutex);
    }
    free_tugs -= mass;
    printf("Ship with mass %d is being lifted %d %d\n", mass, free_tugs, free_docks);
    pthread_mutex_unlock(&tugs_mutex);

    sleep(2);

    pthread_mutex_lock(&tugs_mutex);
    free_tugs += mass;
    pthread_cond_broadcast(&tugs_available);
    printf("Ship with mass %d docked. %d %d\n", mass, free_tugs, free_docks);
    pthread_mutex_unlock(&tugs_mutex);

    sleep(5);

    pthread_mutex_lock(&tugs_mutex);
    while (free_tugs < mass) {
        pthread_cond_wait(&tugs_available, &tugs_mutex);
    }
    free_tugs -= mass;
    pthread_mutex_unlock(&tugs_mutex);

    pthread_mutex_lock(&docks_mutex);
    free_docks++;
    pthread_cond_signal(&dock_available);
    printf("Ship with mass %d leaves the dock %d %d\n", mass, free_tugs, free_docks);
    pthread_mutex_unlock(&docks_mutex);

    sleep(2);

    pthread_mutex_lock(&tugs_mutex);
    free_tugs += mass;
    pthread_cond_broadcast(&tugs_available);
    printf("Ship with mass %d releases the cranes %d %d\n", mass, free_tugs, free_docks);
    pthread_mutex_unlock(&tugs_mutex);
}

int main(int argc, char *argv[]) {
    pthread_t ships[argc];
    for (int i = 0; i < argc; i++) {
        int m = atoi(argv[i]);
        if (pthread_create(&ships[i], NULL, ship_thread, &m) != 0) {
            sleep(1);
            perror("Creating thread");
            exit(1);
        }
    }
    for (int i = 0; i < argc; i++) {
        pthread_join(ships[i], NULL);
    }
    return 0;
}
