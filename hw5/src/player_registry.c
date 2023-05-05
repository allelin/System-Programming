
#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "debug.h"
// #include "client_registry.h"
#include "player_registry.h"
// #include "player.h"
// #include "jeux_globals.h"

struct player_registry {
    // int player_count;
    pthread_mutex_t lock;
    // PLAYER **players;
    PLAYER *head;
};

struct player {
    char *name;
    int ref;
    int rating;
    pthread_mutex_t lock;
    PLAYER *next;
};

PLAYER_REGISTRY *preg_init(void) {
    PLAYER_REGISTRY *preg = malloc(sizeof(PLAYER_REGISTRY));
    if (preg == NULL) {
        debug("malloc failed");
        return NULL;
    }
    if (pthread_mutex_init(&preg->lock, NULL) != 0) {
        free(preg);
        debug("pthread_mutex_init failed");
        return NULL;
    }
    preg->head = NULL;
    return preg;
}

void preg_fini(PLAYER_REGISTRY *preg) {
    if (preg == NULL) {
        debug("preg_fini: preg is NULL");
        return;
    }
    while (preg->head != NULL) {
        PLAYER *player = preg->head;
        player_unref(player, "preg_fini");
        player = player->next;
    }
    pthread_mutex_destroy(&preg->lock);
    free(preg);
}

PLAYER *preg_register(PLAYER_REGISTRY *preg, char *name) {
    debug("preg_register: registering player");
    if (preg == NULL) {
        debug("preg_register: preg is NULL");
        return NULL;
    }
    if (name == NULL) {
        debug("preg_register: name is NULL");
        return NULL;
    }
    pthread_mutex_lock(&preg->lock);
    PLAYER *player = preg->head;
    while (player != NULL) {
        if (strcmp(player->name, name) == 0) {
            player_ref(player, "preg_register");
            pthread_mutex_unlock(&preg->lock);
            debug("preg_register: player found");
            debug("preg_register: relogging");
            return player;
        }
        player = player->next;
    }
    debug("preg_register: player not found");
    debug("preg_register: creating new player");
    player = player_create(name);
    if (player == NULL) {
        pthread_mutex_unlock(&preg->lock);
        return NULL;
    }
    player->next = preg->head;
    preg->head = player;
    pthread_mutex_unlock(&preg->lock);
    return player;
}
