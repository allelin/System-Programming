// #define _GNU_SOURCE
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#include "csapp.h"
#include "debug.h"
#include "server.h"
#include "protocol.h"
#include "jeux_globals.h"
/*
 * Thread function for the thread that handles a particular client.
 *
 * @param  Pointer to a variable that holds the file descriptor for
 * the client connection.  This pointer must be freed once the file
 * descriptor has been retrieved.
 * @return  NULL
 *
 * This function executes a "service loop" that receives packets from
 * the client and dispatches to appropriate functions to carry out
 * the client's requests.  It also maintains information about whether
 * the client has logged in or not.  Until the client has logged in,
 * only LOGIN packets will be honored.  Once a client has logged in,
 * LOGIN packets will no longer be honored, but other packets will be.
 * The service loop ends when the network connection shuts down and
 * EOF is seen.  This could occur either as a result of the client
 * explicitly closing the connection, a timeout in the network causing
 * the connection to be closed, or the main thread of the server shutting
 * down the connection as part of graceful termination.
 */

// CLIENT_REGISTRY *client_registry;
void *jeux_client_service(void *arg) {
    debug("jeux_client_service");
    int client_fd = *(int *)arg;
    free(arg);

    pthread_detach(pthread_self());

    CLIENT *client = creg_register(client_registry, client_fd);
    if (client == NULL) {
        close(client_fd);
        return NULL;
    }
    debug("Client registered");
    JEUX_PACKET_HEADER *header = malloc(sizeof(JEUX_PACKET_HEADER));
    if (header == NULL) {
        creg_unregister(client_registry, client);
        close(client_fd);
        return NULL;
    }

    memset(header, 0, sizeof(JEUX_PACKET_HEADER));
    void *payload = NULL;
    debug("Client service loop");
    while (!(proto_recv_packet(client_fd, header, &payload))) {
        // debug("header->id: %d", header->id);
        debug("header->type: %d", header->type);
        switch (header->type) {
            case JEUX_LOGIN_PKT: {
                // Handle login packet
                debug("Client login");

                if (client_get_player(client) != NULL) {
                    debug("Client already logged in");
                    if (payload != NULL) {
                        free(payload);
                    }
                    client_send_nack(client);
                    break;
                }
                int size = ntohs(header->size);
                char *username = malloc(size + 1);
                memcpy(username, payload, size);
                username[size] = '\0';
                if (payload != NULL) {
                    free(payload);
                }
                // PLAYER *player = player_create(username);
                PLAYER *player = preg_register(player_registry, username);
                if (player == NULL) {
                    debug("Player create failed");
                    creg_unregister(client_registry, client);
                    close(client_fd);
                    free(username);
                    client_send_nack(client);
                    break;
                } 
                // else {
                //     player_ref(player, "jeux_client_service");
                // }
                if (client_login(client, player) == -1) {
                    debug("Client login failed");
                    // header->type = JEUX_NACK_PKT;
                    // header->size = 0;
                    // header->id = 0;
                    // header->role = 0;
                    // client_send_packet(client, header, NULL);
                    client_send_nack(client);
                    // free(player);
                    free(username);
                } else {
                    debug("Client login success");
                    header->type = JEUX_ACK_PKT;
                    header->size = 0;
                    header->id = 0;
                    header->role = 0;
                    client_send_packet(client, header, NULL);
                    debug("client_fd: %d", client_fd);
                    // free(player);
                    free(username);
                }
                break;
            }
            case JEUX_USERS_PKT: {
                // Handle users packet
                debug("Client users");
                if (client_get_player(client) == NULL) {
                    debug("Client not logged in");
                    if (payload != NULL) {
                        free(payload);
                    }
                    client_send_nack(client);
                    break;
                }
                // int char_count = 0;
                PLAYER **players = creg_all_players(client_registry);
                debug("players");
                if (players[0] == NULL) {
                    header->type = JEUX_NACK_PKT;
                    header->size = 0;
                    header->id = 0;
                    header->role = 0;
                    client_send_packet(client, header, NULL);
                    break;
                }
                debug("players[0] != NULL");
                // loop through the players to get total size
                int i = 0;
                char *rating = NULL;
                // debug("char_count: %d", char_count);
                char *pl = malloc(1);
                strcpy(pl, "\0");
                int len = 0;
                while (players[i] != NULL) { 
                    len = snprintf(NULL, 0, "%d", player_get_rating(players[i]));
                    rating = malloc(len + 1);
                    snprintf(rating, len + 1, "%d", player_get_rating(players[i]));
                    size_t current_data = strlen(pl);
                    pl = realloc(pl, current_data + strlen(player_get_name(players[i])) + strlen(rating) + 3);
                    strcat(pl, player_get_name(players[i]));
                    debug("pl: %s", pl);
                    strcat(pl, "\t");
                    debug("BEFORE");
                    debug("rating: %s", rating);
                    strcat(pl, rating);
                    free(rating);
                    debug("here");
                    strcat(pl, "\n");
                    player_unref(players[i], "jeux_client_service");
                    i++;
                }
                debug("pl: %s", pl);
                // debug("char_count: %d", char_count);
                strcat(pl, "\0");
                header->type = JEUX_ACK_PKT;
                header->size = ntohs(strlen(pl) + 1);
                header->id = 0;
                header->role = 0;
                client_send_packet(client, header, pl);
                free(pl);
                free(players);
                if (payload != NULL) {
                    free(payload);
                }
                break;
            }
            case JEUX_INVITE_PKT: {
                // Handle invite packet
                debug("Client invite");
                if (client_get_player(client) == NULL) {
                    debug("Client not logged in");
                    if (payload != NULL) {
                        free(payload);
                    }
                    client_send_nack(client);
                    break;
                }
                // int size = ntohs(header->size);
                // char *username = malloc(size + 1);
                // memcpy(username, payload, size);
                char *username = strdup(payload);
                // username[size] = '\0';
                if (payload != NULL) {
                    free(payload);
                }
                debug("username: %s", username);
                debug("size: %ld", strlen(username));
                CLIENT *target = creg_lookup(client_registry, username);

                if (target == NULL) {
                    debug("Client lookup failed");
                    client_send_nack(client);
                    free(username);
                } else {
                    debug("Client lookup success");
                    header->type = JEUX_ACK_PKT;
                    int role_target = header->role;
                    int role_client = -1;
                    if (role_target == 1) {
                        role_client = 2;
                    } else if (role_target == 2) {
                        role_client = 1;
                    }
                    if (role_target == -1) {
                        client_send_nack(client);
                        free(username);
                        break;
                    }
                    int id = client_make_invitation(client, target, role_client, role_target);
                    if (id == -1) {
                        debug("Client make invitation failed");
                        
                        client_send_nack(client);
                        free(username);
                        client_unref(target, "jeux_client_service");
                        break;
                    }
                    header->id = id;
                    header->size = 0;
                    client_send_packet(client, header, NULL);
                    free(username);
                }
                debug("Client invite end");
                client_unref(target, "jeux_client_service");
                break;
            }
            case JEUX_REVOKE_PKT: {
                // Handle revoke packet
                debug("Client revoke");
                if (client_get_player(client) == NULL) {
                    debug("Client not logged in");
                    if (payload != NULL) {
                        free(payload);
                    }
                    client_send_nack(client);
                    break;
                }
                int id = (header->id);
                if (client_revoke_invitation(client, id) == -1) {
                    debug("Client revoke failed");
                    client_send_nack(client);
                } else {
                    debug("Client revoke success");
                    header->type = JEUX_ACK_PKT;
                    header->size = 0;
                    client_send_packet(client, header, NULL);
                }
                if (payload != NULL) {
                    free(payload);
                }
                break;
            }
           
            case JEUX_DECLINE_PKT: {
                // Handle decline packet
                debug("Client decline");
                if (client_get_player(client) == NULL) {
                    debug("Client not logged in");
                    if (payload != NULL) {
                        free(payload);
                    }
                    client_send_nack(client);
                    break;
                }
                int id = (header->id);
                if (client_decline_invitation(client, id) == -1) {
                    debug("Client decline failed");
                    client_send_nack(client);
                } else {
                    debug("Client decline success");
                    header->type = JEUX_ACK_PKT;
                    header->size = 0;
                    client_send_packet(client, header, NULL);
                }
                free(payload);
                break;
            }
            case JEUX_ACCEPT_PKT: {
                // Handle accept packet
                debug("Client accept");
                if (client_get_player(client) == NULL) {
                    debug("Client not logged in");
                    if (payload != NULL) {
                        free(payload);
                    }
                    client_send_nack(client);
                    break;
                }
                int id = (header->id);
                // get the invitation status by using client_accept_invitation
                char *state = NULL; 
                int status = client_accept_invitation(client, id, &state);
                if (status == -1) {
                    debug("Client accept failed");
                    client_send_nack(client);
                } else {
                    debug("Client accept success");
                    if (state == NULL) {
                        client_send_ack(client, NULL, 0);
                    } else {
                        header->type = JEUX_ACK_PKT;
                        // send ack to urself
                        debug("state: %s", state);
                        header->size = ntohs(strlen(state) + 1);
                        client_send_packet(client, header, state);
                    }
                }
                free(state);
                if (payload != NULL) {
                    free(payload);
                }
                break;
            }
            case JEUX_MOVE_PKT: {
                // Handle move packet
                debug("Client move");
                if (client_get_player(client) == NULL) {
                    debug("Client not logged in");
                    if (payload != NULL) {
                        free(payload);
                    }
                    client_send_nack(client);
                    break;
                }
                int id = (header->id);
                int new_size = ntohs(header->size);
                char *move = malloc(new_size + 1);
                memcpy(move, payload, new_size);
                move[new_size] = '\0';
                if (payload != NULL) {
                    free(payload);
                }
                int status = client_make_move(client, id, move);
                debug("status: %d", status);
                debug("move: %s", move);
                debug("new_size: %d", new_size);
                if (status == -1) {
                    debug("Client move failed");
                    client_send_nack(client);
                } else {
                    debug("Client move success");
                    header->type = JEUX_ACK_PKT;
                    header->size = htons(strlen(move));
                    client_send_ack(client, NULL, 0);
                }
                free(move);
                break;
            }
            case JEUX_RESIGN_PKT: {
                // Handle resign packet
                debug("Client resign");
                if (client_get_player(client) == NULL) {
                    debug("Client not logged in");
                    if (payload != NULL) {
                        free(payload);
                    }
                    client_send_nack(client);
                    break;
                }
                int id = (header->id);
                int status = client_resign_game(client, id);
                if (status == -1) {
                    debug("Client resign failed");
                    client_send_nack(client);
                } else {
                    debug("Client resign success");
                    // header->type = JEUX_ACK_PKT;
                    // header->size = 0;
                    // client_send_packet(client, header, NULL);
                    client_send_ack(client, NULL, 0);
                }
                if (payload != NULL) {
                    free(payload);
                }
                break;
            }
            default: {
                // Handle unknown packet
                break;
            }
        }
        if (!payload) {
            free(payload);
            payload = NULL;
        }
    }
    
    // unref player
    PLAYER *player = client_get_player(client);
    if (player != NULL) {
        player_unref(player, "jeux_client_service");
    }
    client_logout(client);
    creg_unregister(client_registry, client);
    free(header);
    close(client_fd);
    return NULL;
}