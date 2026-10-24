#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <errno.h>
#include <time.h>
#include <poll.h>

#define PORT 5050
#define MAX_FDS 5
#define BUFFER_SIZE 1024
#define MAX_LINE 256
#define NPEERS 3

struct Node {
    int id;
    int term;
    int leader_id;	// this is what will become contested/important to control
	int current_vote;
    int current_term;
	int ping_delay_ms;
	char* peers[NPEERS];
    unsigned long accepted_jobs;
};

const char* nodes[NPEERS] = {"192.168.9.225",
							 "192.168.9.105",
							 "192.168.9.153"};

/****************| HELPER FCNS |****************/

void make_nonblocking(int fd) {
	// Helper function to turn a socket into non-blocking mode
    int flags = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}


static void pause_ms(int milliseconds) {
    struct timespec ts = { 
						   .tv_sec = milliseconds / 1000, 
					       .tv_nsec = (milliseconds % 1000) * 1000000L 
						 };
    while (nanosleep(&ts, &ts) == -1 && errno == EINTR) { }
}


static int read_line(int fd, char *buf, size_t cap) {
    size_t n = 0;
    while (n + 1 < cap) {
        char c;
        ssize_t r = recv(fd, &c, 1, 0);
        if (r <= 0) break;
        if (c == '\n') break;
        buf[n++] = c;
    }
    buf[n] = '\0';
    return n > 0;
}

/****************| END OF HELPER FCNS |****************/

/****************| NODE FUNCTIONS |****************/

static void reply(int fd, const char *text) {
    size_t left = strlen(text);
    while (left) {
        ssize_t sent = send(fd, text, left, 0);
        if (sent <= 0) return;
        left -= (size_t)sent;
        text += sent;
    }
}


int check_args(int id, int port){
	if (id < 0 || id > 2 || port < 1024 || port > 65535) {
        fprintf(stderr, "NODE_ID must be 0..2 and PORT must be 1024..65535\n");
        return 1;
    }
	return 0;
}


static void handle(int fd, struct Node *node) {
    char line[MAX_LINE], out[MAX_LINE];
    int term, leader, ms;
    char job[80];

    if (!read_line(fd, line, sizeof(line))) return;

    if (strncmp(line, "PING", 4) == 0 && (line[4] == '\0' || line[4] == ' ')) {
        pause_ms(node->ping_delay_ms);
        snprintf(out, sizeof(out), "PONG %d\n", node->id);
    } else if (strcmp(line, "STATUS") == 0) {
        snprintf(out, sizeof(out), "STATUS node=%d term=%d leader=%d jobs=%lu delay_ms=%d\n",
            node->id, node->term, node->leader_id, node->accepted_jobs, node->ping_delay_ms);
    } else if (sscanf(line, "DELAY %d", &ms) == 1 && strncmp(line, "DELAY ", 6) == 0) {
        if (ms < 0 || ms > 2000) snprintf(out, sizeof(out), "ERR delay must be 0..2000 ms\n");
        else {
            node->ping_delay_ms = ms;
            snprintf(out, sizeof(out), "OK delay_ms=%d\n", ms);
        }
    } else if (sscanf(line, "LEADER %d %d", &term, &leader) == 2 && strncmp(line, "LEADER ", 7) == 0) {
        if (term < 1 || leader < 0 || leader > 2) {
            snprintf(out, sizeof(out), "ERR invalid leader announcement\n");
        } else if (term > node->term) {
            node->term = term;
            node->leader_id = leader;
            snprintf(out, sizeof(out), "OK term=%d leader=%d\n", term, leader);
            printf("Node %d: adopted term %d, leader %d\n", node->id, term, leader);
        } else if (term == node->term && leader == node->leader_id) {
            snprintf(out, sizeof(out), "OK term=%d leader=%d (unchanged)\n", term, leader);
        } else {
            snprintf(out, sizeof(out), "REJECT stale/conflicting term (current=%d leader=%d)\n", node->term, node->leader_id);
        }
    } else if (sscanf(line, "JOB %d %79s", &term, job) == 2 && strncmp(line, "JOB ", 4) == 0) {
        if (term == node->term && node->leader_id == node->id) {
            // Deliberately naive: local acceptance is NOT a replicated commit.
            node->accepted_jobs++;
            snprintf(out, sizeof(out), "ACCEPT job=%s node=%d term=%d\n", job, node->id, term);
            printf("Node %d: accepted %s under term %d\n", node->id, job, term);
        } else {
            snprintf(out, sizeof(out), "REJECT not leader for term %d (local term=%d leader=%d)\n",
                term, node->term, node->leader_id);
        }
    } else {
        snprintf(out, sizeof(out), "ERR expected PING, STATUS, DELAY, LEADER or JOB\n");
    }
    reply(fd, out);
}


int main(int argc, char*argv[]){
	// parse args 
	if (argc != 3) {
        fprintf(stderr, "Usage: %s NODE_ID PORT\n", argv[0]);
        return 1;
    }
    int id = atoi(argv[1]);
    int port = atoi(argv[2]);
	
	if (check_args(id, port))
		return 1;
	
	// setup node 
	struct Node node = { .id = id, .term = 0, .leader_id = -1 };
    signal(SIGPIPE, SIG_IGN);
	
	int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) { perror("socket"); return 1; }
	
    // Allow quick re-binding to the port if restarted
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
	
	struct sockaddr_in address = {
        .sin_family = AF_INET,
        .sin_addr.s_addr = INADDR_ANY,
        .sin_port = htons(PORT)
    };
	
	if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) { perror("Bind failed"); exit(1); }

    if (listen(server_fd, 3) < 0) { perror("Listen failed"); exit(1); }
    
    // Set the listening socket to non-blocking
    make_nonblocking(server_fd);
    printf("Node %d listening on port %d\n", id, port);
	
	// Array to manage our monitored file descriptors
    struct pollfd fds[MAX_FDS];
    int nfds = 1;

    // Initialize the list: monitor the server socket for incoming connections
    memset(fds, 0, sizeof(fds));
    fds[0].fd = server_fd;
    fds[0].events = POLLIN; 
	
	// Start server loop
	fflush(stdout);
    for (;;) {
        // int client = accept(server, NULL, NULL);
        // if (client < 0) { if (errno == EINTR) continue; perror("accept"); break; }
        // handle(client, &node);
        // close(client);
		// Wait up to 5000ms (5 seconds) for activity
        int activity = poll(fds, nfds, port);
        if (activity < 0) { perror("Poll error"); break; }
        if (activity == 0) { printf("Waiting for activity...\n"); continue; }
		
		int current_size = nfds;
		for (int i = 0; i < current_size; i++) {
            if (!(fds[i].revents & POLLIN)) continue;

            // CASE 1: check new connection incoming
            if (fds[i].fd == server_fd) {
                int client_fd = accept(server_fd, NULL, NULL);
                if (client_fd < 0) {
                    if (errno != EAGAIN && errno != EWOULDBLOCK) perror("Accept error");
                    continue;
                }
				// New client connected on fd
                make_nonblocking(client_fd);
				
				// Make sure we have capacty for it 
				if (nfds < MAX_FDS) {
                    fds[nfds].fd = client_fd;
                    fds[nfds].events = POLLIN;
                    nfds++;
                } else { close(client_fd); } //server full
				
				 // Handle the client 
				handle(client_fd, &node);
				close(client_fd);
				
			} else { 
				// CASE 2: Incoming data
				char buffer[BUFFER_SIZE] = {0};
                int bytes_read = recv(fds[i].fd, buffer, sizeof(buffer) - 1, 0);
				
				// check what data we got from client 
				if (bytes_read < 0) { // ERROR CASE
                    if (errno != EAGAIN && errno != EWOULDBLOCK) {
                        perror("Recv error");
                        close(fds[i].fd);
                        fds[i] = fds[--nfds]; // Remove fd from tracking array
                    }
                } else if (bytes_read == 0) { // EMPTY CASE
                    printf("Client on fd %d disconnected\n", fds[i].fd);
                    close(fds[i].fd);
                    fds[i] = fds[--nfds]; // Remove fd from tracking array
                    i--; // Step back to evaluate the swapped element
                } else { // PARSE INCOMING DATA
                    // Check if the received data contains the phrase "ECHO"
                    if (strstr(buffer, "VOTE") != NULL) {
                        printf("[PEER VOTE RECEIVED] client-%02d\n", fds[i].fd);
                    } else {
                       printf("Client fd %d sent: %s", fds[i].fd, buffer);
                    } 
                }
			}
				
				
	}
    close(server_fd);
    return 1;
	
	}


}
