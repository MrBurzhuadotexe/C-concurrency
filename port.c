#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

#define L_MIEJSC 3
#define L_HOLOWNIKOW 12

pthread_mutex_t miejsca_mtx = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t holowniki_mtx = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t hol_dostepne = PTHREAD_COND_INITIALIZER, miesce_dostepne = PTHREAD_COND_INITIALIZER;

int wolne_miejsca = L_MIEJSC;
int wolne_holowniki = L_HOLOWNIKOW;

void *w_statek(void *m) {
    int masa = *((int *) m);

    pthread_mutex_lock(&miejsca_mtx);
    if (wolne_miejsca == 0){
        pthread_cond_wait(&miesce_dostepne, &miejsca_mtx);
    }
    wolne_miejsca--;
    pthread_mutex_unlock(&miejsca_mtx);

    pthread_mutex_lock(&holowniki_mtx);
    while (wolne_holowniki < masa) {
        pthread_cond_wait(&hol_dostepne, &holowniki_mtx);
    }
    wolne_holowniki -= masa;
    printf("statek o masie %d jest holowany %d %d\n", masa, wolne_holowniki, wolne_miejsca);
    pthread_mutex_unlock(&holowniki_mtx);

    sleep(2);

    pthread_mutex_lock(&holowniki_mtx);
    wolne_holowniki += masa;
    pthread_cond_broadcast(&hol_dostepne);
    printf("Statek o masie %d zadokował. %d %d\n", masa, wolne_holowniki, wolne_miejsca);
    pthread_mutex_unlock(&holowniki_mtx);

    sleep(5);

    pthread_mutex_lock(&holowniki_mtx);
    while (wolne_holowniki < masa) {
        pthread_cond_wait(&hol_dostepne, &holowniki_mtx);
    }
    wolne_holowniki -= masa;
    pthread_mutex_unlock(&holowniki_mtx);

    pthread_mutex_lock(&miejsca_mtx);
    wolne_miejsca++;
    pthread_cond_signal(&miesce_dostepne);
    printf("Statek o masie %d opuszcza dok %d %d\n", masa, wolne_holowniki, wolne_miejsca);
    pthread_mutex_unlock(&miejsca_mtx);

    sleep(2);

    pthread_mutex_lock(&holowniki_mtx);
    wolne_holowniki += masa;
    pthread_cond_broadcast(&hol_dostepne);
    printf("Statek o masie %d uwolnił holowniki %d %d\n", masa, wolne_holowniki, wolne_miejsca);
    pthread_mutex_unlock(&holowniki_mtx);

}

int main(int argc, char *argv[]) {
    pthread_t statki[argc];

    for (int i = 0; i < argc; i++) {
        int m = atoi(argv[i]);
        if (pthread_create(&statki[i], NULL, w_statek, &m) != 0) {
            sleep(1);
            perror("Tworzenie wątku");
            exit(1);
        }
    }
    for (int i = 0; i < argc; i++) {
        pthread_join(statki[i], NULL);
    }
    return 0;
}
