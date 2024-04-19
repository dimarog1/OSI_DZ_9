#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <string.h>
#include <signal.h>

#define SEM_KEY 1234
#define MSG_SIZE 100

int pipefd[2];
int semid;

void handle_sigint(int sig) {
    close(pipefd[0]);
    close(pipefd[1]);
    semctl(semid, 0, IPC_RMID);
    exit(0);
}

int main() {
    char msg[MSG_SIZE];
    pid_t pid;
    struct sembuf sem_op;

    // Создание канала
    if (pipe(pipefd) == -1) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }

    // Создание семафора
    semid = semget(SEM_KEY, 1, 0666 | IPC_CREAT);
    if (semid == -1) {
        perror("semget");
        exit(EXIT_FAILURE);
    }

    // Инициализация семафора
    if (semctl(semid, 0, SETVAL, 1) == -1) {
        perror("semctl");
        exit(EXIT_FAILURE);
    }

    // Регистрация обработчика сигнала
    signal(SIGINT, handle_sigint);

    // Создание дочернего процесса
    pid = fork();
    if (pid == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    }

    if (pid == 0) {  // Дочерний процесс
        close(pipefd[1]);

        while (1) {
            sem_op.sem_num = 0;
            sem_op.sem_op = -1;
            sem_op.sem_flg = 0;
            semop(semid, &sem_op, 1);  // Ожидание семафора

            read(pipefd[0], msg, MSG_SIZE);
            printf("Дочерний процесс получил: %s\n", msg);
        }
    } else {  // Родительский процесс
        close(pipefd[0]);

        int i = 0;
        while (1) {
            sprintf(msg, "Сообщение %d", ++i);
            write(pipefd[1], msg, MSG_SIZE);

            sem_op.sem_num = 0;
            sem_op.sem_op = 1;
            sem_op.sem_flg = 0;
            semop(semid, &sem_op, 1);  // Сигнализация семафора

            sleep(1);  // Задержка в одну секунду
        }
    }

    return 0;
}