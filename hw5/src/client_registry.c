#include "client_registry.h"

#include <pthread.h>
#include <semaphore.h>
#include <stdbool.h>
#include <stdlib.h>

#include "csapp.h"
#include "debug.h"

#define MAX_FD_SIZE 1024
struct client_registry {
    int client_count;
    pthread_mutex_t mutex;
    sem_t sem;
    int client_fds[MAX_FD_SIZE];
    CLIENT **clients;
};

CLIENT_REGISTRY *creg_init() {
    debug("initializing client registry");
    CLIENT_REGISTRY *cr = malloc(sizeof(CLIENT_REGISTRY));
    if (cr == NULL) {
        free(cr);
        return NULL;
    }
    cr->client_count = 0;
    cr->clients = calloc(MAX_CLIENTS, sizeof(CLIENT *));
    if (cr->clients == NULL) {
        free(cr->clients);
        free(cr);
        return NULL;
    }
    if (pthread_mutex_init(&cr->mutex, NULL) < 0) {
        free(cr->clients);
        free(cr);
        return NULL;
    }
    if (sem_init(&cr->sem, 0, 0) < 0) {
        free(cr->clients);
        free(cr);
        return NULL;
    }
    for (int i = 0; i < MAX_FD_SIZE; i++) {
        cr->client_fds[i] = -1;
    }
    return cr;
}

/*
 * Finalize a client registry, freeing all associated resources.
 * This method should not be called unless there are no currently
 * registered clients.
 *
 * @param cr  The client registry to be finalized, which must not
 * be referenced again.
 */
void creg_fini(CLIENT_REGISTRY *cr) {
    debug("finalizing client registry");
    pthread_mutex_destroy(&cr->mutex);
    sem_destroy(&cr->sem);
    free(cr->clients);
    free(cr);
}

/*
 * Register a client file descriptor.
 * If successful, returns a reference to the the newly registered CLIENT,
 * otherwise NULL.  The returned CLIENT has a reference count of one.
 *
 * @param cr  The client registry.
 * @param fd  The file descriptor to be registered.
 * @return a reference to the newly registered CLIENT, if registration
 * is successful, otherwise NULL.
 */
CLIENT *creg_register(CLIENT_REGISTRY *cr, int fd) {
    debug("registering client");
    if (cr->client_fds[fd] != -1) {
        return NULL;
    }
    debug("r1");
    pthread_mutex_lock(&cr->mutex);
    // check if the array is full
    if (cr->client_count == MAX_CLIENTS) {
        pthread_mutex_unlock(&cr->mutex);
        debug("client registry is full");
        return NULL;
    }
    int i;
    debug("r2");
    for (i = 0; i < MAX_CLIENTS; i++) {
        if (cr->clients[i] == NULL) {
            break;
        }
    }
    cr->clients[i] = client_create(cr, fd);
    cr->client_fds[fd] = 0;
    cr->client_count++;
    pthread_mutex_unlock(&cr->mutex);
    debug("r3");
    return cr->clients[i];
}

/*
 * Unregister a CLIENT, removing it from the registry.
 * The client reference count is decreased by one to account for the
 * pointer discarded by the client registry.  If the number of registered
 * clients is now zero, then any threads that are blocked in
 * creg_wait_for_empty() waiting for this situation to occur are allowed
 * to proceed.  It is an error if the CLIENT is not currently registered
 * when this function is called.
 *
 * @param cr  The client registry.
 * @param client  The CLIENT to be unregistered.
 * @return 0  if unregistration succeeds, otherwise -1.
 */
int creg_unregister(CLIENT_REGISTRY *cr, CLIENT *client) {
    debug("unregister");
    if (cr->client_fds[client_get_fd(client)] == -1) {
        return -1;
    }
    pthread_mutex_lock(&cr->mutex);
    int i;
    // remove client from clients array and shift all clients to the left
    for (i = 0; i < MAX_CLIENTS; i++) {
        if (cr->clients[i] == client) {
            cr->clients[i] = NULL;
            break;
        }
    }
    // move all clients to the left
    for (int j = i; j < MAX_CLIENTS - 1; j++) {
        cr->clients[j] = cr->clients[j + 1];
    }
    // set last client to null
    cr->clients[MAX_CLIENTS - 1] = NULL;

    cr->client_fds[client_get_fd(client)] = -1;
    cr->client_count--;
    client_unref(client, "unregister");
    if (cr->client_count == 0) {
        sem_post(&cr->sem);
    }
    pthread_mutex_unlock(&cr->mutex);
    return 0;
}

/*
 * Given a username, return the CLIENT that is logged in under that
 * username.  The reference count of the returned CLIENT is
 * incremented by one to account for the reference returned.
 *
 * @param cr  The registry in which the lookup is to be performed.
 * @param user  The username that is to be looked up.
 * @return the CLIENT currently registered under the specified
 * username, if there is one, otherwise NULL.
 */
CLIENT *creg_lookup(CLIENT_REGISTRY *cr, char *user) {
    debug("lookup");
    if (!cr || !user) {
        return NULL;
    }
    pthread_mutex_lock(&cr->mutex);

    // iterate through all clients
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (cr->clients[i] != NULL) {
            CLIENT *client = cr->clients[i];
            if (client == NULL) {
                continue;
            }
            PLAYER *player = client_get_player(client);
            if (player == NULL) {
                continue;
            }
            char *name = player_get_name(player);
            debug("name: %ld", strlen(name));
            debug("user: %ld", strlen(user));
            if (strcmp(name, user) == 0) {
                client_ref(client, "lookup");
                pthread_mutex_unlock(&cr->mutex);
                return client;
            }
        }
        if (i == MAX_CLIENTS - 1) {
            pthread_mutex_unlock(&cr->mutex);
            return NULL;
        }
    }
    pthread_mutex_unlock(&cr->mutex);
    return NULL;
}

/*
 * Return a list of all currently logged in players.  The result is
 * returned as a malloc'ed array of PLAYER pointers, with a NULL
 * pointer marking the end of the array.  It is the caller's
 * responsibility to decrement the reference count of each of the
 * entries and to free the array when it is no longer needed.
 *
 * @param cr  The registry for which the set of usernames is to be
 * obtained.
 * @return the list of players as a NULL-terminated array of pointers.
 */
PLAYER **creg_all_players(CLIENT_REGISTRY *cr) {
    debug("creg_all_players");
    if (!cr) {
        return NULL;
    }
    PLAYER **players = NULL;
    pthread_mutex_lock(&cr->mutex);
    int i;
    for (i = 0; i < MAX_CLIENTS; i++) {
        CLIENT *client = cr->clients[i];
        if (client == NULL) {
            break;
        }
        if (client != NULL) {
            debug("client is not null");
            PLAYER *player = client_get_player(client);
            players = realloc(players, sizeof(PLAYER *) * (i + 1));
            debug("realloced");
            if (players == NULL) {
                debug("realloc failed");
                pthread_mutex_unlock(&cr->mutex);
                return NULL;
            }
            if (player != NULL) {
                debug("player is not null");

                players[i] = player;
                debug("added player to array");
                player_ref(player, "all_players");
            } else {
                debug("player is null");
                players[i] = NULL;
                // debug("added null to array");
            }
        }
    }
    players = realloc(players, sizeof(PLAYER *) * (i + 1));
    // add NULL to the end of the array
    debug("i is %d", i);
    players[i] = NULL;
    pthread_mutex_unlock(&cr->mutex);
    return players;
}

/*
 * A thread calling this function will block in the call until
 * the number of registered clients has reached zero, at which
 * point the function will return.  Note that this function may be
 * called concurrently by an arbitrary number of threads.
 *
 * @param cr  The client registry.
 */
void creg_wait_for_empty(CLIENT_REGISTRY *cr) {
    debug("wait");
    if (!cr) {
        return;
    }
    if (cr->client_count == 0) {
        sem_post(&cr->sem);
    }
    sem_wait(&cr->sem);
}

/*
 * Shut down (using shutdown(2)) all the sockets for connections
 * to currently registered clients.  The clients are not unregistered
 * by this function.  It is intended that the clients will be
 * unregistered by the threads servicing their connections, once
 * those server threads have recognized the EOF on the connection
 * that has resulted from the socket shutdown.
 *
 * @param cr  The client registry.
 */
void creg_shutdown_all(CLIENT_REGISTRY *cr) {
    debug("shutdown");
    if (!cr) {
        return;
    }
    pthread_mutex_lock(&cr->mutex);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (cr->clients[i] != NULL) {
            int fd = client_get_fd(cr->clients[i]);
            shutdown(fd, SHUT_RDWR);
            close(fd);
        }
    }
    pthread_mutex_unlock(&cr->mutex);
}