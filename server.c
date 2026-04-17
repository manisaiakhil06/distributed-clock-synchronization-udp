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
#include <openssl/rand.h>
#include <openssl/hmac.h>

#define PORT 8080
#define CERT_FILE "server.crt"
#define KEY_FILE  "server.key"

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
    CPU_SET(0, &cpuset);
    sched_setaffinity(0, sizeof(cpu_set_t), &cpuset);
}

static void tune_socket(int fd) {
    int buf = 4 * 1024 * 1024;
    setsockopt(fd, SOL_SOCKET, SO_RCVBUF, &buf, sizeof(buf));
    setsockopt(fd, SOL_SOCKET, SO_SNDBUF, &buf, sizeof(buf));

    int tos = IPTOS_LOWDELAY;
    setsockopt(fd, IPPROTO_IP, IP_TOS, &tos, sizeof(tos));
}

static unsigned char cookie_secret[16];

static int generate_cookie(SSL *ssl, unsigned char *cookie, unsigned int *cookie_len) {
    unsigned char buf[64];
    struct sockaddr_in peer;

    BIO_dgram_get_peer(SSL_get_rbio(ssl), &peer);

    memcpy(buf, &peer.sin_addr, sizeof(peer.sin_addr));
    memcpy(buf + sizeof(peer.sin_addr), &peer.sin_port, sizeof(peer.sin_port));

    HMAC(EVP_sha1(), cookie_secret, sizeof(cookie_secret),
         buf, sizeof(peer.sin_addr) + sizeof(peer.sin_port),
         cookie, cookie_len);

    return 1;
}

static int verify_cookie(SSL *ssl, const unsigned char *cookie, unsigned int cookie_len) {
    unsigned char expected[EVP_MAX_MD_SIZE];
    unsigned int expected_len;

    generate_cookie(ssl, expected, &expected_len);

    return (cookie_len == expected_len &&
            memcmp(cookie, expected, expected_len) == 0);
}

int main() {
    set_realtime();

    SSL_library_init();
    SSL_load_error_strings();

    RAND_bytes(cookie_secret, sizeof(cookie_secret));

    SSL_CTX *ctx = SSL_CTX_new(DTLS_server_method());

    if (!SSL_CTX_use_certificate_file(ctx, CERT_FILE, SSL_FILETYPE_PEM) ||
        !SSL_CTX_use_PrivateKey_file(ctx, KEY_FILE, SSL_FILETYPE_PEM)) {
        printf("Error loading certificate/key\n");
        return 1;
    }

    SSL_CTX_set_cookie_generate_cb(ctx, generate_cookie);
    SSL_CTX_set_cookie_verify_cb(ctx, verify_cookie);

    int fd = socket(AF_INET, SOCK_DGRAM, 0);
    tune_socket(fd);

    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    addr.sin_addr.s_addr = INADDR_ANY;

    bind(fd, (struct sockaddr *)&addr, sizeof(addr));

    printf("Server running on port %d...\n", PORT);

    while (1) {
        SSL *ssl = SSL_new(ctx);
        BIO *bio = BIO_new_dgram(fd, BIO_NOCLOSE);
        SSL_set_bio(ssl, bio, bio);
        SSL_set_options(ssl, SSL_OP_COOKIE_EXCHANGE);

        struct sockaddr_in client_addr;

        printf("\nWaiting for client...\n");

        while (DTLSv1_listen(ssl, (BIO_ADDR *)&client_addr) <= 0);

        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, sizeof(client_ip));
        int client_port = ntohs(client_addr.sin_port);

        printf("Client detected: %s:%d\n", client_ip, client_port);

        if (SSL_accept(ssl) <= 0) {
            ERR_print_errors_fp(stderr);
            SSL_free(ssl);
            continue;
        }

        printf("Handshake successful!\n");

        time_msg_t msg;

        while (1) {
            int n = SSL_read(ssl, &msg, sizeof(msg));
            if (n <= 0) break;

            printf("\n[%s:%d] Ping %d received\n", client_ip, client_port, msg.seq);

            msg.t2 = now();
            printf("t2 (Server Receive Time): %.6f\n", msg.t2);

            msg.t3 = now();
            printf("t3 (Server Send Time)   : %.6f\n", msg.t3);

            SSL_write(ssl, &msg, sizeof(msg));
        }

        printf("Client disconnected: %s:%d\n", client_ip, client_port);

        SSL_shutdown(ssl);
        SSL_free(ssl);
    }

    close(fd);
    SSL_CTX_free(ctx);
    return 0;
}
