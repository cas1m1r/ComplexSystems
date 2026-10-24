#define _POSIX_C_SOURCE 200809L
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define MAX_CLIENTS 16
#define INPUT_SIZE 1024
#define OUTPUT_SIZE 4096

struct Client {
    int fd;
    int peer_closed;
    char input[INPUT_SIZE];
    size_t input_len;
    char output[OUTPUT_SIZE];
    size_t output_len;
    size_t output_sent;
};

unsigned long requests = 0; // One event-loop thread owns this state

int make_nonblocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    return flags < 0 ? -1 : fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

void close_client(struct Client *c) {
    if (c->fd >= 0) close(c->fd);
    *c = (struct Client){.fd = -1};
}

int queue_reply(struct Client *c, const char *line) {
    char reply[128];
    requests++;
    if (!strcmp(line, "PING")) snprintf(reply, sizeof(reply), "PONG\n");
    else if (!strcmp(line, "STATUS")) snprintf(reply, sizeof(reply), "requests=%lu\n", requests);
    else snprintf(reply, sizeof(reply), "ERR unknown command\n");
    size_t len = strlen(reply);
    if (len > sizeof(c->output) - c->output_len) return -1;
    memcpy(c->output + c->output_len, reply, len);
    c->output_len += len;
    return 0;
}

int read_client(struct Client *c) {
    for (;;) {
        size_t space = sizeof(c->input) - 1 - c->input_len;
        if (!space) return -1; // Bounded line: do not overrun or wait forever
        ssize_t n = recv(c->fd, c->input + c->input_len, space, 0);
        if (n > 0) {
            c->input_len += (size_t)n;
            c->input[c->input_len] = '\0';
            char *newline;
            while ((newline = memchr(c->input, '\n', c->input_len)) != NULL) {
                size_t used = (size_t)(newline - c->input) + 1;
                *newline = '\0';
                if (queue_reply(c, c->input) < 0) return -1;
                memmove(c->input, c->input + used, c->input_len - used);
                c->input_len -= used;
                c->input[c->input_len] = '\0';
            }
        } else if (n == 0) {
            c->peer_closed = 1;
            return 0; // Incomplete tail is discarded; complete replies still drain
        } else if (errno == EINTR) {
            continue;
        } else if (errno == EAGAIN || errno == EWOULDBLOCK) {
            return 0; // No bytes now; retain the partial line
        } else {
            return -1;
        }
    }
}

int write_client(struct Client *c) {
    while (c->output_sent < c->output_len) {
        ssize_t n = send(c->fd, c->output + c->output_sent, c->output_len - c->output_sent, MSG_NOSIGNAL);
        if (n > 0) c->output_sent += (size_t)n;
        else if (n < 0 && errno == EINTR) continue;
        else if (n < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) return 0;
        else return -1;
    }
    c->output_len = c->output_sent = 0;
    return 0;
}

int main(int argc, char **argv) {
    char *end;
    long port = argc == 2 ? strtol(argv[1], &end, 10) : 5050;
    if (argc > 2 || (argc == 2 && (!argv[1][0] || *end)) || port < 1 || port > 65535) {
        fprintf(stderr, "usage: %s [port:1..65535]\n", argv[0]);
        return 2;
    }
    int listener = socket(AF_INET, SOCK_STREAM, 0), one = 1;
    if (listener < 0) { perror("socket"); return 1; }
    struct sockaddr_in address = {.sin_family = AF_INET, .sin_addr.s_addr = htonl(INADDR_LOOPBACK), .sin_port = htons((unsigned short)port)};
    if (setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one)) < 0 ||
        make_nonblocking(listener) < 0 || bind(listener, (struct sockaddr *)&address, sizeof(address)) < 0 || listen(listener, MAX_CLIENTS) < 0) {
        perror("listener setup");
        close(listener);
        return 1;
    }
    struct Client clients[MAX_CLIENTS];
    for (int i = 0; i < MAX_CLIENTS; i++) clients[i] = (struct Client){.fd = -1};
    printf("Listening on 127.0.0.1:%ld\n", port);
    fflush(stdout);
    for (;;) {
        struct pollfd fds[MAX_CLIENTS + 1] = {{.fd = listener, .events = POLLIN}};
        for (int i = 0; i < MAX_CLIENTS; i++) {
            fds[i + 1].fd = clients[i].fd;
            fds[i + 1].events = clients[i].peer_closed ? 0 : POLLIN;
            if (clients[i].output_len > clients[i].output_sent) fds[i + 1].events |= POLLOUT;
        }
        int ready = poll(fds, MAX_CLIENTS + 1, -1);
        if (ready < 0) {
            if (errno == EINTR) continue;
            perror("poll");
            break;
        }
        if (fds[0].revents & POLLIN) {
            for (;;) {
                int fd = accept(listener, NULL, NULL);
                if (fd < 0) {
                    if (errno == EINTR) continue;
                    if (errno != EAGAIN && errno != EWOULDBLOCK) perror("accept");
                    break;
                }
                int slot;
                for (slot = 0; slot < MAX_CLIENTS && clients[slot].fd >= 0; slot++) {}
                if (slot == MAX_CLIENTS || make_nonblocking(fd) < 0) close(fd);
                else clients[slot] = (struct Client){.fd = fd};
            }
        }
        for (int i = 0; i < MAX_CLIENTS; i++) {
            struct Client *c = &clients[i];
            short events = fds[i + 1].revents;
            if (c->fd < 0 || !events) continue;
            if (events & (POLLERR | POLLNVAL)) { close_client(c); continue; }
            if ((events & (POLLIN | POLLHUP)) && read_client(c) < 0) { close_client(c); continue; }
            if (c->output_len > c->output_sent && write_client(c) < 0) { close_client(c); continue; }
            if (c->peer_closed && c->output_len == c->output_sent) close_client(c);
        }
    }
    for (int i = 0; i < MAX_CLIENTS; i++) close_client(&clients[i]);
    close(listener);
    return 1;
}
