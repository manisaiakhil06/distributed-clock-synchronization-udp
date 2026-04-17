#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <time.h>
#include <sched.h>
#include <netinet/ip.h>

#include <openssl/ssl.h>
#include <openssl/err.h>

#define PORT 8080
#define PING_COUNT 10

typedef struct __attribute__((packed)) {
    uint32_t seq;
    double t2;
    double t3;
} time_msg_t;

static double now(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
    return ts.tv_sec + ts.tv_nsec * 1e-9;
}

static void set_realtime() {
    struct sched_param param;
    param.sched_priority = sched_get_priority_max(SCHED_FIFO);
    sched_setscheduler(0, SCHED_FIFO, &param);

    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(1, &cpuset);
    sched_setaffinity(0, sizeof(cpu_set_t), &cpuset);
}

static void tune_socket(int fd) {
    int buf = 4 * 1024 * 1024;
    setsockopt(fd, SOL_SOCKET, SO_RCVBUF, &buf, sizeof(buf));
    setsockopt(fd, SOL_SOCKET, SO_SNDBUF, &buf, sizeof(buf));

    int tos = IPTOS_LOWDELAY;
    setsockopt(fd, IPPROTO_IP, IP_TOS, &tos, sizeof(tos));

#ifdef SO_BUSY_POLL
    int busy = 50;
    setsockopt(fd, SOL_SOCKET, SO_BUSY_POLL, &busy, sizeof(busy));
#endif
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: %s <server_ip>\n", argv[0]);
        return 1;
    }

    set_realtime();

    SSL_library_init();
    SSL_load_error_strings();

    SSL_CTX *ctx = SSL_CTX_new(DTLS_client_method());
    SSL_CTX_set_verify(ctx, SSL_VERIFY_NONE, NULL);

    int fd = socket(AF_INET, SOCK_DGRAM, 0);
    tune_socket(fd);

    struct sockaddr_in srv = {0};
    srv.sin_family = AF_INET;
    srv.sin_port = htons(PORT);
    inet_pton(AF_INET, argv[1], &srv.sin_addr);

    connect(fd, (struct sockaddr *)&srv, sizeof(srv));

    BIO *bio = BIO_new_dgram(fd, BIO_CLOSE);
    BIO_ctrl(bio, BIO_CTRL_DGRAM_SET_CONNECTED, 0, &srv);
    BIO_ctrl(bio, BIO_CTRL_DGRAM_SET_MTU, 1200, NULL);

    SSL *ssl = SSL_new(ctx);
    SSL_set_bio(ssl, bio, bio);

    printf("Connecting to server...\n");

    if (SSL_connect(ssl) <= 0) {
        ERR_print_errors_fp(stderr);
        return 1;
    }

    printf("Handshake successful!\n\n");

    time_msg_t msg;

    for (int i = 1; i <= PING_COUNT; i++) {
        msg.seq = i;
        msg.t2 = 0;
        msg.t3 = 0;

        printf("========== Ping %d ==========\n", i);

        double t1 = now();
        printf("t1 (Client Send Time)   : %.6f\n", t1);

        if (SSL_write(ssl, &msg, sizeof(msg)) <= 0) {
            printf("Send error\n\n");
            continue;
        }

        int n = SSL_read(ssl, &msg, sizeof(msg));
        double t4 = now();

        if (n <= 0) {
            printf("Receive error\n\n");
            continue;
        }

        printf("t2 (Server Receive Time): %.6f\n", msg.t2);
        printf("t3 (Server Send Time)   : %.6f\n", msg.t3);
        printf("t4 (Client Receive Time): %.6f\n", t4);

        double delay = (t4 - t1) - (msg.t3 - msg.t2);
        double offset = ((msg.t2 - t1) + (msg.t3 - t4)) / 2.0;

        printf("Delay  : %.3f ms\n", delay * 1000);
        printf("Offset : %.3f ms\n\n", offset * 1000);
    }

    SSL_shutdown(ssl);
    SSL_free(ssl);
    SSL_CTX_free(ctx);
    return 0;
}
