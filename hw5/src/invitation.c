
#include "debug.h"
#include "csapp.h"
#include "client_registry.h"
#include "game.h"
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include "client.h"
#include "invitation.h"
struct invitation {
    CLIENT *source;
    CLIENT *target;
    INVITATION_STATE state;
    GAME_ROLE source_role;
    GAME_ROLE target_role;
    GAME *game;
    int ref;
    pthread_mutex_t lock;
};

INVITATION *inv_create(CLIENT *source, CLIENT *target, GAME_ROLE source_role, GAME_ROLE target_role) {
    INVITATION *inv = malloc(sizeof(INVITATION));
    if (inv == NULL) {
        free(inv);
        debug("malloc failed");
        return NULL;
    }
    // return a error if the source and target are the same
    if (source == target) {
        free(inv);
        debug("source and target are the same");
        return NULL;
    }
    inv->source = source;
    inv->target = target;
    inv->state = INV_OPEN_STATE;
    inv->source_role = source_role;
    inv->target_role = target_role;
    inv->ref = 0;
    inv->game = NULL;
    
    if (pthread_mutex_init(&inv->lock, NULL) != 0) {
        free(inv);
        debug("pthread_mutex_init failed");
        return NULL;
    }
    inv_ref(inv, "inv_create");
    client_ref(source, "inv_create");
    client_ref(target, "inv_create");
    return inv;
}

/*
 * Increase the reference count on an invitation by one.
 *
 * @param inv  The INVITATION whose reference count is to be increased.
 * @param why  A string describing the reason why the reference count is
 * being increased.  This is used for debugging printout, to help trace
 * the reference counting.
 * @return  The same INVITATION object that was passed as a parameter.
 */
INVITATION *inv_ref(INVITATION *inv, char *why) {
    if (inv == NULL) {
        debug("inv_ref: inv is NULL");
        return NULL;
    }
    pthread_mutex_lock(&inv->lock);
    inv->ref++;
    debug("inv_ref: %s", why);
    debug("NUMBER OF REFSSSSSSSSSS: %d", inv->ref);
    pthread_mutex_unlock(&inv->lock);
    return inv;
}

/*
 * Decrease the reference count on an invitation by one.
 * If after decrementing, the reference count has reached zero, then the
 * invitation and its contents are freed.
 *
 * @param inv  The INVITATION whose reference count is to be decreased.
 * @param why  A string describing the reason why the reference count is
 * being decreased.  This is used for debugging printout, to help trace
 * the reference counting.
 *
 */
void inv_unref(INVITATION *inv, char *why) {
    if (inv == NULL) {
        debug("inv_ref: inv is NULL");
        exit(EXIT_FAILURE);
    }
    pthread_mutex_lock(&inv->lock);
    inv->ref--;
    debug("REF: %d", inv->ref);
    debug("inv_unref: %s", why);

    if (inv->ref == 0) {
        

        client_unref(inv->source, why);
        client_unref(inv->target, why);
        if (inv->game != NULL) {
            game_unref(inv->game, why);
        }
        pthread_mutex_unlock(&inv->lock);
        pthread_mutex_destroy(&inv->lock);
        free(inv);
        debug("inv_unref: %s", why);
        return;
    }
    pthread_mutex_unlock(&inv->lock);
}

/*
 * Get the CLIENT that is the source of an INVITATION.
 * The reference count of the returned CLIENT is NOT incremented,
 * so the CLIENT reference should only be regarded as valid as
 * long as the INVITATION has not been freed.
 *
 * @param inv  The INVITATION to be queried.
 * @return the CLIENT that is the source of the INVITATION.
 */
CLIENT *inv_get_source(INVITATION *inv) {
    if (inv == NULL) {
        return NULL;
    }
    return inv->source;
}

/*
 * Get the CLIENT that is the target of an INVITATION.
 * The reference count of the returned CLIENT is NOT incremented,
 * so the CLIENT reference should only be regarded as valid if
 * the INVITATION has not been freed.
 *
 * @param inv  The INVITATION to be queried.
 * @return the CLIENT that is the target of the INVITATION.
 */
CLIENT *inv_get_target(INVITATION *inv) {
    if (inv == NULL) {
        return NULL;
    }
    return inv->target;
}

/*
 * Get the GAME_ROLE to be played by the source of an INVITATION.
 *
 * @param inv  The INVITATION to be queried.
 * @return the GAME_ROLE played by the source of the INVITATION.
 */
GAME_ROLE inv_get_source_role(INVITATION *inv) {
    if (inv == NULL) {
        exit(EXIT_FAILURE);
    }
    return inv->source_role;
}

/*
 * Get the GAME_ROLE to be played by the target of an INVITATION.
 *
 * @param inv  The INVITATION to be queried.
 * @return the GAME_ROLE played by the target of the INVITATION.
 */
GAME_ROLE inv_get_target_role(INVITATION *inv) {
    if (inv == NULL) {
        exit(EXIT_FAILURE);
    }
    return inv->target_role;
}

/*
 * Get the GAME (if any) associated with an INVITATION.
 * The reference count of the returned GAME is NOT incremented,
 * so the GAME reference should only be regarded as valid as long
 * as the INVITATION has not been freed.
 *
 * @param inv  The INVITATION to be queried.
 * @return the GAME associated with the INVITATION, if there is one,
 * otherwise NULL.
 */
GAME *inv_get_game(INVITATION *inv) {
    if(inv == NULL) {
        return NULL;
    }
    return inv->game;
}

/*
 * Accept an INVITATION, changing it from the OPEN to the
 * ACCEPTED state, and creating a new GAME.  If the INVITATION was
 * not previously in the the OPEN state then it is an error.
 *
 * @param inv  The INVITATION to be accepted.
 * @return 0 if the INVITATION was successfully accepted, otherwise -1.
 */
int inv_accept(INVITATION *inv) {
    if (inv == NULL) {
        return -1;
    }
    pthread_mutex_lock(&inv->lock);
    if (inv->state != INV_OPEN_STATE) {
        pthread_mutex_unlock(&inv->lock);
        return -1;
    }
    inv->state = INV_ACCEPTED_STATE;

    // create a new game
    GAME *game = game_create();
    if (game == NULL) {
        pthread_mutex_unlock(&inv->lock);
        return -1;
    }
    inv->game = game;
    // set up the source and target roles
    client_ref(inv->source, "inv_accept");
    client_ref(inv->target, "inv_accept");
    
    pthread_mutex_unlock(&inv->lock);
    return 0;
}


/*
 * Close an INVITATION, changing it from either the OPEN state or the
 * ACCEPTED state to the CLOSED state.  If the INVITATION was not previously
 * in either the OPEN state or the ACCEPTED state, then it is an error.
 * If INVITATION that has a GAME in progress is closed, then the GAME
 * will be resigned by a specified player.
 *
 * @param inv  The INVITATION to be closed.
 * @param role  This parameter identifies the GAME_ROLE of the player that
 * should resign as a result of closing an INVITATION that has a game in
 * progress.  If NULL_ROLE is passed, then the invitation can only be
 * closed if there is no game in progress.
 * @return 0 if the INVITATION was successfully closed, otherwise -1.
 */
int inv_close(INVITATION *inv, GAME_ROLE role) {
    if (inv == NULL) {
        return -1;
    }
    pthread_mutex_lock(&inv->lock);
    debug("INV STATE NUMBER: %d", inv->state);
    debug("role: %d", role);
    if (role != NULL_ROLE) {
        if (inv->state != INV_ACCEPTED_STATE && inv->state != INV_OPEN_STATE) {
            pthread_mutex_unlock(&inv->lock);
            return -1;
        }
        if (inv->state == INV_OPEN_STATE) {
            inv->state = INV_CLOSED_STATE;
            pthread_mutex_unlock(&inv->lock);
            return 0;
        }
        if (inv->state == INV_ACCEPTED_STATE) {
            if (game_is_over(inv->game) == 0) {
                debug("game is not over");
                game_resign(inv->game, role);
            }
            inv->state = INV_CLOSED_STATE;
            pthread_mutex_unlock(&inv->lock);
            return 0;
        }
    } else {
        if (inv->state == INV_OPEN_STATE) {
            inv->state = INV_CLOSED_STATE;
            pthread_mutex_unlock(&inv->lock);
            return 0;
        }
        if (inv->state != INV_OPEN_STATE) {
            pthread_mutex_unlock(&inv->lock);
            return -1;
        }
    }
    // if (inv->state != INV_ACCEPTED_STATE && inv->state != INV_OPEN_STATE) {
    //     pthread_mutex_unlock(&inv->lock);
    //     return -1;
    // }
//     if (role == NULL_ROLE) {
//         if (inv->game != NULL) {
//             if (game_is_over(inv->game) == 0) {
//                 debug("game is not over");
//                 pthread_mutex_unlock(&inv->lock);
//                 return -1;
//             }
//         } 
//     }
//     if (inv->game != NULL) {
//         if (game_is_over(inv->game) == 0) {
//             if (game_resign(inv->game, role) != 0) {
//                 pthread_mutex_unlock(&inv->lock);
//                 return -1;
//             }
//         }
//     }
//     debug("WE ARE HERE");
//     inv->state = INV_CLOSED_STATE;
    // client_unref(inv->source, "inv_close");
    // client_unref(inv->target, "inv_close");
    // inv_unref(inv, "inv_close");
    // pthread_mutex_destroy(&inv->lock);
    
    pthread_mutex_unlock(&inv->lock);
    return -1;
}