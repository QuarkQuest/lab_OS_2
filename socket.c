#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>

volatile sig_atomic_t SigHup = 0;

void sigHupHandler(int sig) {
    SigHup = 1;
}

int main() {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
        perror("socket");
        exit(1);
    }

    struct sockaddr_in addr = {
        .sin_family = AF_INET,
        .sin_port = htons(1234),
        .sin_addr.s_addr = INADDR_ANY
    };

    if (bind(sock, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
        perror("bind");
        close(sock);
        exit(1);
    }

    if (listen(sock, SOMAXCONN) == -1) {
        perror("listen");
        close(sock);
        exit(1);
    }

    struct sigaction sa;
    sigaction(SIGHUP, NULL, &sa);
    sa.sa_handler = sigHupHandler;
    sa.sa_flags |= SA_RESTART;
    sigaction(SIGHUP, &sa, NULL);

    sigset_t blockedMask, origMask;
    sigemptyset(&blockedMask);
    sigaddset(&blockedMask, SIGHUP);
    sigprocmask(SIG_BLOCK, &blockedMask, &origMask);

    printf("Сервер запущен на 1234 и PID: %d\n", getpid());

    int clients = -1;

    while (1) {
        fd_set fds;
        FD_ZERO(&fds);
        FD_SET(sock, &fds);
        int maxFd = sock;

        if (clients != -1) {
            FD_SET(clients, &fds);
            if (clients > maxFd) maxFd = clients;
        }

        int ready = pselect(maxFd + 1, &fds, NULL, NULL, NULL, &origMask);
        if (ready == -1) {
            if (errno == EINTR) {

                if (SigHup) {
                    printf("Получен сигнал SIGHUP\n");
                    SigHup = 0;
                }
                continue;
            }
            perror("2");
            break;
        }

        if (FD_ISSET(sock, &fds)) {
            int newClient = accept(sock, NULL, NULL);
            if (newClient == -1) {
                perror("3");
            } else {
                printf("Новое подключение: fd=%d\n", newClient);
                if (clients == -1) {
                    clients = newClient;
                    printf("Оставлено соединение: fd=%d\n", clients);
                } else {
                    close(newClient);
                    printf("Закрыто лишнее соединение: fd=%d\n", newClient);
                }
            }
        }

        if (clients != -1 && FD_ISSET(clients, &fds)) {
            char buffer[1024];
            ssize_t bytesRead = recv(clients, buffer, sizeof(buffer) - 1, 0);
            if (bytesRead > 0) {
                buffer[bytesRead] = '\0';
                printf("Данные от клиента %d: %s", clients, buffer);
            } else {
                printf("Клиент %d отключился\n", clients);
                close(clients);
                clients = -1;
            }
        }
    }

    if (clients != -1) close(clients);
    close(sock);
    return 0;
}