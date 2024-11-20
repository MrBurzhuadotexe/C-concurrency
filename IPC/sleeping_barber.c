#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/msg.h>
#include <stdbool.h>
#include <errno.h>
#include <sys/wait.h>


#define KEY 45309  
#define B 2 //number of barbers
#define WR 1 //number of seats in the waiting room
#define N 1 //number of barber chairs
#define C 4 //number of clients
#define HAIRCUT_PRICE 27

const int denominations[3] = {1, 2, 5};


static struct sembuf sem_buf;


void V(int semid, int semnum) {
    sem_buf.sem_num = semnum;
    sem_buf.sem_op = 1;
    sem_buf.sem_flg = 0;
    if (semop(semid, &sem_buf, 1) == -1) {
        perror("Raising semaphore");
        exit(1);
    }
}

void P(int semid, int semnum) {
    sem_buf.sem_num = semnum;
    sem_buf.sem_op = -1;
    sem_buf.sem_flg = 0;
    if (semop(semid, &sem_buf, 1) == -1) {
        perror("Lowering semaphore");
        exit(1);
    }
}


struct waiting_room_slot {
    long mtype;
    int client_id;
};
#define FREE 1
#define OCCUPIED 2

void barber(int id, int (*shm_buf)[C + 1]);

void client(int id, int (*shm_buf)[C + 1]);

int payment(int id_k, int (*shm_buf)[C + 1]);

int sum(const int money[3]);

void change(int id_k, int cash, int (*shm_buf)[C + 1]);

static int shmid, msgid, semid;
struct waiting_room_slot place;



int main() {
    int (*shm_buf)[C + 1];
    shmid = shmget(KEY, (C + 1) * sizeof(int[3]), IPC_CREAT | 0600);
    if (shmid == -1) {
        perror("Creating shared memory segment");
        exit(1);
    }
    shm_buf = shmat(shmid, NULL, 0);

    shm_buf[0][0] = 5;
    shm_buf[0][1] = 5;

    msgid = msgget(KEY, IPC_CREAT | 0600);
    if (msgid == -1) {
        perror("Creating message queue");
        exit(1);
    }
    else {
        place.mtype = FREE;
        for (int i = 0; i < WR; i++) {
            if (msgsnd(msgid, &place, sizeof(place.client_id), 0) == -1) {
                perror("Sending empty message");
                exit(1);
            }
        }

    }


    semid = semget(KEY, C + 3, IPC_CREAT | 0600);
    if (semid == -1) {
        perror("Creating semaphore array");
        exit(1);
    }

    if (semctl(semid, 0, SETVAL, (int) N) == -1) {
        perror("Setting semaphore value for chairs");
        exit(1);
    }
    for (int i = 1; i <= C; i++) {
        if (semctl(semid, i, SETVAL, (int) 0) == -1) {
            perror("Setting semaphore value for clients");
            exit(1);
        }

    }
    if (semctl(semid, C + 1, SETVAL, (int) 1) == -1) {
        perror("Setting semaphore value for cash");
        exit(1);
    }
    if (semctl(semid, C + 2, SETVAL, (int) 0) == -1) {
        perror("Setting semaphore value for change");
        exit(1);
    }

    for (int i = 1; i <= B; i++) {
        if (fork() == 0) {
            barber(i, shm_buf);
        }
        sleep(1);
    }
    for (int i = 1; i <= C; i++) {
        if (fork() == 0) {
            client(i, shm_buf);
        }
        sleep(3);
    }
    wait(NULL);
}

void barber(int id, int (*shm_buf)[C + 1]) {
    int cash;

    while(true){
        printf("Barber %d is sleeping...\n", id);
        if (msgrcv(msgid, &place, sizeof(place.client_id), OCCUPIED, 0) == -1) {
            perror("Receiving client");
            exit(1);
        }


        int client_id = place.client_id;

        V(semid, client_id);
        printf("Barber %d accepted client %d\n", id, client_id);

        P(semid, 0);
        printf("Barber %d found an empty chair\n", id);

        cash = payment(client_id, shm_buf);
        printf("Client %d paid barber %d amount %d\n", client_id, id, cash);

        sleep(5);
        printf("Barber %d is cutting hair for client %d \n", id, client_id);
        sleep(5);

        printf("Client %d has a new haircut\n", client_id);
        V(semid, 0);

        if (cash > HAIRCUT_PRICE) {
            change(client_id, cash, shm_buf);
        }
        V(semid, client_id);
    }
}

void client(int id, int (*shm_buf)[C + 1]) {
    while (true) {
        printf("Client %d is earning money\n", id);
        for(int i = 0; i < 6; i++){
            shm_buf[id][2]++;
            sleep(3);
        }

        printf("Client %d is going to the barbershop\n", id);
        sleep(5);

        if (msgrcv(msgid, &place, sizeof(place.client_id), FREE, IPC_NOWAIT) == -1) {
            printf("Client %d didn't find a place in the waiting room\n", id);

        } else {
            place.client_id = id;
            place.mtype = OCCUPIED;
            if (msgsnd(msgid, &place, sizeof(place.client_id), 0) == -1) {
                perror("Occupying place");
                exit(1);
            }
            printf("Client %d is waiting to be served\n", id);

            P(semid, id);
            place.mtype = FREE;
            if (msgsnd(msgid, &place, sizeof(place.client_id), 0) == -1) {
                perror("Releasing place from waiting room");
                exit(1);
            }

            P(semid, id);

        }
        sleep(8);
    }
}

int sum(const int money[3]) {
    return money[0] * 1 + money[1] * 2 + money[2] * 5;
}

int payment(int id_k, int (*shm_buf)[C + 1]) {
    int payment_sum = 0;
    int payment_denominations[3] = {0};

    for (int i = 0; i < 3; i++) {
        while (shm_buf[id_k][i] > 0) {
            if (payment_sum >= HAIRCUT_PRICE) {
                i = 3;
                break;
            }
            payment_sum += denominations[i];
            payment_denominations[i]++;
            shm_buf[id_k][i]--;
        }
    }


    P(semid, C + 1);
    for (int i = 0; i < 3; i++) {
        shm_buf[0][i] += payment_denominations[i];
    }
    V(semid, C + 2);
    V(semid, C + 1);


    return payment_sum;
}

void change(int id_k, int cash, int (*shm_buf)[C + 1]) {
    int change = cash - HAIRCUT_PRICE, required_change, change_denominations[2], cash_denominations[2];

    printf("Client %d is waiting for change: %d\n", id_k, change);

    while (true) {
        P(semid, C + 2);
        P(semid, C + 1);


        required_change = change;
        change_denominations[0] = 0;
        change_denominations[1] = 0;
        cash_denominations[0] = shm_buf[0][0];
        cash_denominations[1] = shm_buf[0][1];

        for (int i = 1; i >= 0; i--) {
            while(cash_denominations[i] > 0 && required_change - denominations[i] >= 0){
                required_change -= denominations[i];
                change_denominations[i]++;
                cash_denominations[i]--;
            }
        }
        if (required_change == 0) {
            for (int i = 0; i < 2; i++) {
                shm_buf[0][i] -= change_denominations[i];
                shm_buf[id_k][i] += change_denominations[i];
            }
            printf("Client %d received change: %d coins of 1, %d of 2\n", id_k, change_denominations[0],
                   change_denominations[1]);
            V(semid, C + 1);
            break;
        }
        else{
            V(semid, C + 1);
        }
    }
}
