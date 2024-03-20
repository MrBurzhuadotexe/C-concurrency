#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/msg.h>
#include <stdbool.h>
#include <errno.h>
#include <sys/wait.h>
#include <time.h>

#define KEY 45306
#define FRYZJERZY 2
#define POCZEKALNIA 1
#define FOTELE 1
#define KLIENCI 3
#define CENA_FRYZURY 27

const int nominaly[3] = {1, 2, 5};


static struct sembuf sem_buf;

void podnies(int semid, int semnum) {
    sem_buf.sem_num = semnum;
    sem_buf.sem_op = 1;
    sem_buf.sem_flg = 0;
    if (semop(semid, &sem_buf, 1) == -1) {
        perror("Podnoszenie semafora");
        exit(1);
    }
}

void opusc(int semid, int semnum) {
    sem_buf.sem_num = semnum;
    sem_buf.sem_op = -1;
    sem_buf.sem_flg = 0;
    if (semop(semid, &sem_buf, 1) == -1) {
        perror("Opuszczenie semafora");
        exit(1);
    }
}


struct miejsce_w_poczekalnie {
    long mtype;
    int id_klienta;
};
#define WOLNE 1
#define ZAJETE 2

void fryzjer(int id, int (*shm_buf)[KLIENCI + 1]);

void klient(int id, int (*shm_buf)[KLIENCI + 1]);

int oplata(int id_k, int (*shm_buf)[KLIENCI + 1]);

int suma(const int pieniadze[3]);

void reszta(int id_k, int gotowka, int (*shm_buf)[KLIENCI + 1]);

static int shmid, msgid, semid;
struct miejsce_w_poczekalnie miejsce;



int main() {
    int (*shm_buf)[KLIENCI + 1];
    shmid = shmget(KEY, (KLIENCI + 1) * sizeof(int[3]), IPC_CREAT | 0600);
    if (shmid == -1) {
        perror("Utworzenie segmentu pamieci wspoldzielonej");
        exit(1);
    }
    shm_buf = shmat(shmid, NULL, 0);

    shm_buf[0][0] = 5;
    shm_buf[0][1] = 5;

    for (int i = 0; i < 4; i++){
        for (int j = 0; j < 3; j++){
            printf("%d ", shm_buf[i][j]);
        }
        printf("\n");
    }

    msgid = msgget(KEY, IPC_CREAT | 0600);
    if (msgid == -1) {
        perror("Utworzenie kolejki komunikatow");
        exit(1);
    }
    else {
        miejsce.mtype = WOLNE;
        for (int i = 0; i < POCZEKALNIA; i++) {
            if (msgsnd(msgid, &miejsce, sizeof(miejsce.id_klienta), 0) == -1) {
                perror("Wyslanie pustego komunikatu");
                exit(1);
            }
        }

    }


    semid = semget(KEY, KLIENCI + 3, IPC_CREAT | 0600);
    if (semid == -1) {
        perror("Utworzenie tablicy semaforow");
        exit(1);
    }

    if (semctl(semid, 0, SETVAL, (int) FOTELE) == -1) {
        perror("Nadanie wartosci semaforowi dla foteli");
        exit(1);
    }
    for (int i = 1; i <= KLIENCI; i++) {
        if (semctl(semid, i, SETVAL, (int) 0) == -1) {
            perror("Nadanie wartosci semaforowi dla klienta");
            exit(1);
        }

    }
    if (semctl(semid, KLIENCI + 1, SETVAL, (int) 1) == -1) {
        perror("Nadanie wartosci semaforowi dla kasy");
        exit(1);
    }
    if (semctl(semid, KLIENCI + 2, SETVAL, (int) 0) == -1) {
        perror("Nadanie wartosci semaforowi dla reszty");
        exit(1);
    }

    for (int i = 1; i <= FRYZJERZY; i++) {
        if (fork() == 0) {
            fryzjer(i, shm_buf);
        }
        sleep(1);
    }
    for (int i = 1; i <= KLIENCI; i++) {
        if (fork() == 0) {
            klient(i, shm_buf);
        }
        sleep(7);
    }
    wait(NULL);
}

void fryzjer(int id, int (*shm_buf)[KLIENCI + 1]) {
    int gotowka;

    while(true){
        printf("Fryzjer %d śpi...\n", id);
        if (msgrcv(msgid, &miejsce, sizeof(miejsce.id_klienta), ZAJETE, 0) == -1) {
            perror("Odebranie klienta");
            exit(1);
        }


        int id_klienta = miejsce.id_klienta;

        podnies(semid, id_klienta);
        printf("Fryzjer %d przyjął klienta %d\n", id, id_klienta);

        opusc(semid, 0);
        printf("Fryzjer %d znalazł wolny fotel\n", id);

        gotowka = oplata(id_klienta, shm_buf);
        printf("Klient %d zapłacił fryzjeru %d kwotę %d\n", id_klienta, id, gotowka);

        sleep(5);
        printf("Fryzjer %d strzyże klienta %d \n", id, id_klienta);
        sleep(5);

        printf("Klient %d otrzymał nową fryzurę\n\n", id_klienta);
        podnies(semid, 0);

        if (gotowka > CENA_FRYZURY) {
            reszta(id_klienta, gotowka, shm_buf);
        }
        podnies(semid, id_klienta);
    }
}

void klient(int id, int (*shm_buf)[KLIENCI + 1]) {
    while (true) {
        printf("Klient %d zarabia pieniądzy\n", id);
        for(int i = 0; i < 6; i++){
            shm_buf[id][2]++;
            sleep(3);
        }

        printf("Klient %d idzie do fryzjerów\n", id);
        sleep(5);

        if (msgrcv(msgid, &miejsce, sizeof(miejsce.id_klienta), WOLNE, IPC_NOWAIT) == -1) {
            printf("Klient %d nie znalazł miejsca w poczekalnie\n", id);

        } else {
            miejsce.id_klienta = id;
            miejsce.mtype = ZAJETE;
            if (msgsnd(msgid, &miejsce, sizeof(miejsce.id_klienta), 0) == -1) {
                perror("Zajęcie miejsca");
                exit(1);
            }
            printf("Klient %d czeka na obsługę\n", id);

            opusc(semid, id);
            miejsce.mtype = WOLNE;
            if (msgsnd(msgid, &miejsce, sizeof(miejsce.id_klienta), 0) == -1) {
                perror("Zwolnienie miejsca z poczekalnie");
                exit(1);
            }

            opusc(semid, id);

        }
        sleep(8);
    }
}

int suma(const int pieniadze[3]) {
    return pieniadze[0] * 1 + pieniadze[1] * 2 + pieniadze[2] * 5;
}

int oplata(int id_k, int (*shm_buf)[KLIENCI + 1]) {


    int plata_suma = 0;
    int plata_nominaly[3] = {0};


    for (int i = 0; i < 3; i++) {
        while (shm_buf[id_k][i] > 0) {
            if (plata_suma >= CENA_FRYZURY) {
                i = 3;
                break;
            }
            plata_suma += nominaly[i];
            plata_nominaly[i]++;
            shm_buf[id_k][i]--;
        }
    }

    for (int i = 0; i < 4; i++){
        for (int j = 0; j < 3; j++){
            printf("%d ", shm_buf[i][j]);
        }
        printf("\n");
    }

    printf("\n\n\n");

    opusc(semid, KLIENCI + 1);
    for (int i = 0; i < 3; i++) {
        shm_buf[0][i] += plata_nominaly[i];
    }
    podnies(semid, KLIENCI + 2);
    podnies(semid, KLIENCI + 1);


    return plata_suma;
}

void reszta(int id_k, int gotowka, int (*shm_buf)[KLIENCI + 1]) {
    int reszta = gotowka - CENA_FRYZURY, wymagana_reszta, reszta_nominaly[2], kasa_nominaly[2];

    printf("Klient %d czeka na resztę: %d\n", id_k, reszta);

    while (true) {
        opusc(semid, KLIENCI + 2);
        opusc(semid, KLIENCI + 1);

        for (int i = 0; i < 4; i++){
            for (int j = 0; j < 3; j++){
                printf("%d ", shm_buf[i][j]);
            }
            printf("\n");
        }

        wymagana_reszta = reszta;
        reszta_nominaly[0] = 0;
        reszta_nominaly[1] = 0;
        kasa_nominaly[0] = shm_buf[0][0];
        kasa_nominaly[1] = shm_buf[0][1];

        for (int i = 1; i >= 0; i--) {
            while(kasa_nominaly[i] > 0 && wymagana_reszta - nominaly[i] >= 0){
                wymagana_reszta -= nominaly[i];
                reszta_nominaly[i]++;
                kasa_nominaly[i]--;
            }
        }
        if (wymagana_reszta == 0) {
            for (int i = 0; i < 2; i++) {
                shm_buf[0][i] -= reszta_nominaly[i];
                shm_buf[id_k][i] += reszta_nominaly[i];
            }
            printf("Klient %d otrzymał resztę: %d monety po 1, %d po 2\n", id_k, reszta_nominaly[0],
                   reszta_nominaly[1]);
            podnies(semid, KLIENCI + 1);
            break;
        }
        else{
            podnies(semid, KLIENCI + 1);
        }
    }
}
