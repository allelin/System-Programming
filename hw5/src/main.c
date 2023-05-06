// #define _XOPEN_SOURCE 700
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>
#include <signal.h>
#include <ctype.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <getopt.h>
#include "debug.h"
#include "protocol.h"
#include "server.h"
#include "client_registry.h"
#include "player_registry.h"
#include "jeux_globals.h"
#include "csapp.h"
#include <pthread.h>
#ifdef DEBUG
int _debug_packets_ = 1;
#endif

static void terminate(int status);
static void sighup_handler(int sig) {
    terminate(EXIT_SUCCESS);
}
static void sigpipe_handler(int sig) {
    // do nothing
    debug("sigpipe_handler: sigpipe received");
}

void *thread(void *vargp);
/*
 * "Jeux" game server.
 *
 * Usage: jeux <port>
 */
int main(int argc, char* argv[]){
    // Option processing should be performed here.
    // Option '-p <port>' is required in order to specify the port number
    // on which the server should listen.
    
    // go through the arguments and check for a -p flag and then get the port number after (double check if this is all numbers)
    char *port = NULL;
    int opt;
    int p = 0;
    while ((opt = getopt(argc, argv, "p:")) != -1) {
        switch (opt) {
            case 'p':
                p = 1;
                // check if the port number is all numbers
                for (int i = 0; i < strlen(optarg); i++) {
                    if (!isdigit(optarg[i])) {
                        fprintf(stderr, "Usage: %s -p <port>\n", argv[0]);
                        exit(EXIT_FAILURE);
                    }
                }
                port = (optarg);
                break;
            default:
                fprintf(stderr, "Usage: %s -p <port>\n", argv[0]);
                exit(EXIT_FAILURE);
        }
    }

    if (!p || !port) {
        fprintf(stderr, "Usage: %s -p <port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    // Perform required initializations of the client_registry and
    // player_registry.
    client_registry = creg_init();
    player_registry = preg_init();

    // TODO: Set up the server socket and enter a loop to accept connections
    // on this socket.  For each connection, a thread should be started to
    // run function jeux_client_service().  In addition, you should install
    // a SIGHUP handler, so that receipt of SIGHUP will perform a clean
    // shutdown of the server.

    // install the sigup handler
    struct sigaction sa;
    sa.sa_handler = sighup_handler;
    sa.sa_flags = 0;
    sigemptyset(&sa.sa_mask);
    if (sigaction(SIGHUP, &sa, NULL) == -1) {
        fprintf(stderr, "sigaction error: %s\n", strerror(errno));
        terminate(EXIT_FAILURE);
    }

    // install the sigpipe handler
    struct sigaction sa2;
    sa2.sa_handler = sigpipe_handler;
    sa2.sa_flags = 0;
    sigemptyset(&sa2.sa_mask);
    if (sigaction(SIGPIPE, &sa2, NULL) == -1) {
        fprintf(stderr, "sigaction error: %s\n", strerror(errno));
        terminate(EXIT_FAILURE);
    }
    int listenfd, *connfd; 
    socklen_t clientlen; 
    struct sockaddr_storage clientaddr;
    pthread_t tid;

    // change the port into a string
    listenfd = open_listenfd(port);
    while (1) {
        clientlen = sizeof(struct sockaddr_storage);
        connfd = malloc(sizeof(int));
        *connfd = accept(listenfd, (SA *)&clientaddr, &clientlen);
        if (connfd < 0) {
            if (errno == EINTR) {
                continue;
            } else {
                free(connfd);
                fprintf(stderr, "accept error: %s\n", strerror(errno));
                exit(1);
            }
        }
        pthread_create(&tid, NULL, thread, connfd);
    }
    fprintf(stderr, "You have to finish implementing main() "
	    "before the Jeux server will function.\n");
    terminate(EXIT_FAILURE);
}

// thread function
void *thread(void *vargp) {
    // int connfd = *((int *)vargp);
    // pthread_detach(pthread_self());
    jeux_client_service(vargp);
    // free(vargp);
    // close(connfd);
    return NULL;
}

/*
 * Function called to cleanly shut down the server.
 */
void terminate(int status) {
    // Shutdown all client connections.
    // This will trigger the eventual termination of service threads.
    creg_shutdown_all(client_registry);
    
    debug("%ld: Waiting for service threads to terminate...", pthread_self());
    creg_wait_for_empty(client_registry);
    debug("%ld: All service threads terminated.", pthread_self());

    // Finalize modules.
    creg_fini(client_registry);
    preg_fini(player_registry);

    debug("%ld: Jeux server terminating", pthread_self());
    exit(status);
}
