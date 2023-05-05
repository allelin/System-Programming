#include <semaphore.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "debug.h"
#include "player.h"

struct player {
    char *name;
    int ref;
    int rating;
    pthread_mutex_t lock;
    PLAYER *next;
};

PLAYER *player_create(char *name) {
    PLAYER *player = malloc(sizeof(PLAYER));
    if (player == NULL) {
        debug("malloc failed");
        return NULL;
    }
    player->name = strdup(name);
    player->ref = 0;
    player->rating = PLAYER_INITIAL_RATING;
    if (pthread_mutex_init(&player->lock, NULL) != 0) {
        free(player);
        debug("pthread_mutex_init failed");
        return NULL;
    }
    player_ref(player, "player_create");
    return player;
}

PLAYER *player_ref(PLAYER *player, char *why) {
    if (player == NULL) {
        debug("player_ref: player is NULL");
        return NULL;
    }
    pthread_mutex_lock(&player->lock);
    player->ref++;
    debug("PLAYER REF: %d", player->ref);
    debug("player_ref: %s", why);
    pthread_mutex_unlock(&player->lock);
    return player;
}

/*
 * Decrease the reference count on a PLAYER by one.
 * If after decrementing, the reference count has reached zero, then the
 * PLAYER and its contents are freed.
 *
 * @param player  The PLAYER whose reference count is to be decreased.
 * @param why  A string describing the reason why the reference count is
 * being decreased.  This is used for debugging printout, to help trace
 * the reference counting.
 *
 */
void player_unref(PLAYER *player, char *why) {
    if (player == NULL) {
        debug("player_unref: player is NULL");
        return;
    }
    pthread_mutex_lock(&player->lock);
    player->ref--;
    debug("player_unref: %s", why);
    if (player->ref == 0) {
        pthread_mutex_unlock(&player->lock);
        pthread_mutex_destroy(&player->lock);
        free(player->name);
        free(player);
        return;
    }
    pthread_mutex_unlock(&player->lock);
}

/*
 * Get the username of a player.
 *
 * @param player  The PLAYER that is to be queried.
 * @return the username of the player.
 */
char *player_get_name(PLAYER *player) {
    if (player == NULL) {
        debug("player_get_name: player is NULL");
        return NULL;
    }
    return player->name;
}

/*
 * Get the rating of a player.
 *
 * @param player  The PLAYER that is to be queried.
 * @return the rating of the player.
 */
int player_get_rating(PLAYER *player) {
    if (player == NULL) {
        debug("player_get_rating: player is NULL");
        return -1;
    }
    return player->rating;
}

/*
 * Post the result of a game between two players.
 * To update ratings, we use a system of a type devised by Arpad Elo,
 * similar to that used by the US Chess Federation.
 * The player's ratings are updated as follows:
 * Assign each player a score of 0, 0.5, or 1, according to whether that
 * player lost, drew, or won the game.
 * Let S1 and S2 be the scores achieved by player1 and player2, respectively.
 * Let R1 and R2 be the current ratings of player1 and player2, respectively.
 * Let E1 = 1/(1 + 10**((R2-R1)/400)), and
 *     E2 = 1/(1 + 10**((R1-R2)/400))
 * Update the players ratings to R1' and R2' using the formula:
 *     R1' = R1 + 32*(S1-E1)
 *     R2' = R2 + 32*(S2-E2)
 *
 * @param player1  One of the PLAYERs that is to be updated.
 * @param player2  The other PLAYER that is to be updated.
 * @param result   0 if draw, 1 if player1 won, 2 if player2 won.
 */
void player_post_result(PLAYER *player1, PLAYER *player2, int result) {
    double score1 = 0;
    double score2 = 0;
    pthread_mutex_lock(&player1->lock);
    pthread_mutex_lock(&player2->lock);

    double rating1 = player1->rating;
    double rating2 = player2->rating;  

    if (result == 0) {
        score1 = 0.5;
        score2 = 0.5;
    } else if (result == 1) {
        score1 = 1;
        score2 = 0;
    } else if (result == 2) {
        score1 = 0;
        score2 = 1;
    } else {
        debug("player_post_result: result is not 0, 1, or 2");
        return;
    }

    double expected1 = 1 / (1 + pow(10, (rating2 - rating1) / 400.0));
    double expected2 = 1 / (1 + pow(10, (rating1 - rating2) / 400.0));

    player1->rating = rating1 + (int)(32 * (score1 - expected1));
    player2->rating = rating2 + (int)(32 * (score2 - expected2));

    pthread_mutex_unlock(&player1->lock);
    pthread_mutex_unlock(&player2->lock);
}