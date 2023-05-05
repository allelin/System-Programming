#include "game.h"

#include <ctype.h>
#include <math.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "debug.h"

struct game {
    int state;
    int ref;
    GAME_ROLE whose_turn;
    GAME_ROLE winner;
    pthread_mutex_t lock;
    char board[3][3];
};

struct game_move {
    char move;
    GAME_ROLE role;
};

GAME *game_create(void) {
    GAME *game = malloc(sizeof(GAME));
    if (game == NULL) {
        debug("malloc failed");
        return NULL;
    }
    game->state = 0;
    game->ref = 0;
    game->whose_turn = FIRST_PLAYER_ROLE;
    game->winner = NULL_ROLE;
    if (pthread_mutex_init(&game->lock, NULL) != 0) {
        free(game);
        debug("pthread_mutex_init failed");
        return NULL;
    }
    // clear the board
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            game->board[i][j] = ' ';
        }
    }
    game_ref(game, "game_create");
    return game;
}

GAME *game_ref(GAME *game, char *why) {
    if (game == NULL) {
        debug("game_ref: game is NULL");
        return NULL;
    }
    pthread_mutex_lock(&game->lock);
    game->ref++;
    debug("game_ref: %s", why);
    pthread_mutex_unlock(&game->lock);
    return game;
}

void game_unref(GAME *game, char *why) {
    if (game == NULL) {
        debug("game_unref: game is NULL");
        return;
    }
    pthread_mutex_lock(&game->lock);
    game->ref--;
    debug("game_unref: %s", why);
    if (game->ref == 0) {
        pthread_mutex_unlock(&game->lock);
        pthread_mutex_destroy(&game->lock);
        free(game);
        return;
    }
    pthread_mutex_unlock(&game->lock);
}

int game_apply_move(GAME *game, GAME_MOVE *move) {
    if (game == NULL) {
        debug("game_apply_move: game is NULL");
        return -1;
    }
    if (move == NULL) {
        debug("game_apply_move: move is NULL");
        return -1;
    }
    debug("game_apply_move: applying move %c", move->move);
    int number = move->move - '0';
    if (number < 1 || number > 9) {
        debug("game_apply_move: move is not a number between 1 and 9");
        return -1;
    }
    int row = (int)floor((number - 1.0) / 3.0);
    int col = (number - 1) % 3;
    pthread_mutex_lock(&game->lock);
    debug("game_apply_move: locked game mutex");
    debug("move: %c", move->move);
    debug("row: %d, col: %d", row, col);
    if (game->state != 0) {
        debug("game_apply_move: game is not in progress");
        pthread_mutex_unlock(&game->lock);
        return -1;
    }
    if (game->whose_turn != move->role) {
        debug("game_apply_move: it is not this player's turn");
        pthread_mutex_unlock(&game->lock);
        return -1;
    }
    // if (game->board[r][c] != ' ') {
    //     debug("game_apply_move: move is already occupied");
    //     pthread_mutex_unlock(&game->lock);
    //     return -1;
    // }
    if (row < 0 || row > 2 || col < 0 || col > 2) {
        debug("game_apply_move: move is out of bounds");
        pthread_mutex_unlock(&game->lock);
        return -1;
    }
    if (game->board[row][col] != ' ') {
        debug("game_apply_move: move is already occupied");
        pthread_mutex_unlock(&game->lock);
        return -1;
    }
    game->board[row][col] = move->role == FIRST_PLAYER_ROLE ? 'X' : 'O';
    game->whose_turn = move->role == FIRST_PLAYER_ROLE ? SECOND_PLAYER_ROLE : FIRST_PLAYER_ROLE;

    // check if the game is over aka if a person won the tic tac toe game
    // check rows
    for (int i = 0; i < 3; i++) {
        if (game->board[i][0] == game->board[i][1] && game->board[i][1] == game->board[i][2]) {
            if (game->board[i][0] == 'X') {
                game->winner = FIRST_PLAYER_ROLE;
                game->state = 1;
                game->whose_turn = NULL_ROLE;
                pthread_mutex_unlock(&game->lock);
                return 0;
            } else if (game->board[i][0] == 'O') {
                game->winner = SECOND_PLAYER_ROLE;
                game->state = 1;
                game->whose_turn = NULL_ROLE;
                pthread_mutex_unlock(&game->lock);
                return 0;
            }
        }
    }
    // check columns
    for (int i = 0; i < 3; i++) {
        if (game->board[0][i] == game->board[1][i] && game->board[1][i] == game->board[2][i]) {
            if (game->board[0][i] == 'X') {
                game->winner = FIRST_PLAYER_ROLE;
                game->state = 1;
                game->whose_turn = NULL_ROLE;
                pthread_mutex_unlock(&game->lock);
                return 0;
            } else if (game->board[0][i] == 'O') {
                game->winner = SECOND_PLAYER_ROLE;
                game->state = 1;
                game->whose_turn = NULL_ROLE;
                pthread_mutex_unlock(&game->lock);
                return 0;
            }
        }
    }
    // check diagonals
    if (game->board[0][0] == game->board[1][1] && game->board[1][1] == game->board[2][2]) {
        if (game->board[0][0] == 'X') {
            game->winner = FIRST_PLAYER_ROLE;
            game->state = 1;
            game->whose_turn = NULL_ROLE;
            pthread_mutex_unlock(&game->lock);
            return 0;
        } else if (game->board[0][0] == 'O') {
            game->winner = SECOND_PLAYER_ROLE;
            game->state = 1;
            game->whose_turn = NULL_ROLE;
            pthread_mutex_unlock(&game->lock);
            return 0;
        }
    }
    // check diagonals
    if (game->board[0][2] == game->board[1][1] && game->board[1][1] == game->board[2][0]) {
        if (game->board[0][2] == 'X') {
            game->winner = FIRST_PLAYER_ROLE;
            game->state = 1;
            game->whose_turn = NULL_ROLE;
            pthread_mutex_unlock(&game->lock);
            return 0;
        } else if (game->board[0][2] == 'O') {
            game->winner = SECOND_PLAYER_ROLE;
            game->state = 1;
            game->whose_turn = NULL_ROLE;
            pthread_mutex_unlock(&game->lock);
            return 0;
        }
    }
    // check for a tie
    for (int i = 0; i < 3; i++) {
        if (game->board[i][0] == ' ' || game->board[i][1] == ' ' || game->board[i][2] == ' ') {
            pthread_mutex_unlock(&game->lock);
            return 0;
        }
    }
    // if we get here, then the game is a tie
    game->state = 1;
    game->whose_turn = NULL_ROLE;
    game->winner = NULL_ROLE;
    pthread_mutex_unlock(&game->lock);
    return 0;
}

int game_resign(GAME *game, GAME_ROLE role) {
    if (game == NULL) {
        debug("game_resign: game is NULL");
        return -1;
    }
    pthread_mutex_lock(&game->lock);
    if (game->state != 0) {
        debug("game_resign: game is not in progress");
        pthread_mutex_unlock(&game->lock);
        return -1;
    }
    game->state = 1;
    game->whose_turn = NULL_ROLE;
    game->winner = role == FIRST_PLAYER_ROLE ? SECOND_PLAYER_ROLE : FIRST_PLAYER_ROLE;
    pthread_mutex_unlock(&game->lock);
    return 0;
}

char *game_unparse_state(GAME *game) {
    if (game == NULL) {
        debug("game_unparse_state: game is NULL");
        return NULL;
    }
    pthread_mutex_lock(&game->lock);
    char *state = malloc(6 * 5 * sizeof(char) + 1);
    if (state == NULL) {
        debug("game_unparse_state: malloc failed");
        pthread_mutex_unlock(&game->lock);
        return NULL;
    }
    //set the state to a empty string
    memset(state, 0, 6 * 5 * sizeof(char));
    for (int i = 0; i < 3; i++) {
    char row[8];
    snprintf(row, sizeof(row), "%c|%c|%c\n", game->board[i][0], game->board[i][1], game->board[i][2]);
    strcat(state, row);
    if (i != 2) {
        strcat(state, "-----\n");
    }
}
    pthread_mutex_unlock(&game->lock);
    return state;
}

int game_is_over(GAME *game) {
    if (game == NULL) {
        debug("game_is_over: game is NULL");
        return -1;
    }
    // pthread_mutex_lock(&game->lock);
    int state = game->state;
    // pthread_mutex_unlock(&game->lock);
    return state;
}

GAME_ROLE game_get_winner(GAME *game) {
    if (game == NULL) {
        debug("game_get_winner: game is NULL");
        return NULL_ROLE;
    }
    // pthread_mutex_lock(&game->lock);
    GAME_ROLE winner = game->winner;
    // pthread_mutex_unlock(&game->lock);
    return winner;
}

GAME_MOVE *game_parse_move(GAME *game, GAME_ROLE role, char *str) {
    if (game == NULL) {
        debug("game_parse_move: game is NULL");
        return NULL;
    }
    if (str == NULL) {
        debug("game_parse_move: str is NULL");
        return NULL;
    }
    pthread_mutex_lock(&game->lock);
    if (role != NULL_ROLE) {
        if (game->whose_turn != role) {
            debug("game_parse_move: it is not this player's turn");
            pthread_mutex_unlock(&game->lock);
            return NULL;
        }
    }
    if (game->state != 0) {
        debug("game_parse_move: game is not in progress");
        pthread_mutex_unlock(&game->lock);
        return NULL;
    }
    if (strlen(str) != 1) {
        debug("game_parse_move: str is not of length 1");
        pthread_mutex_unlock(&game->lock);
        return NULL;
    }
    if (isdigit(str[0]) == 0) {
        debug("game_parse_move: str is not a digit");
        pthread_mutex_unlock(&game->lock);
        return NULL;
    }
    if (str[0] < '1' || str[0] > '9') {
        debug("game_parse_move: str is not in the range 1-9");
        pthread_mutex_unlock(&game->lock);
        return NULL;
    }
    GAME_MOVE *move = malloc(sizeof(GAME_MOVE));
    if (move == NULL) {
        debug("game_parse_move: malloc failed");
        pthread_mutex_unlock(&game->lock);
        return NULL;
    }
    move->move = str[0];
    move->role = role;
    pthread_mutex_unlock(&game->lock);
    return move;
}

char *game_unparse_move(GAME_MOVE *move) {
    if (move == NULL) {
        debug("game_unparse_move: move is NULL");
        return NULL;
    }
    char *str = malloc(2 * sizeof(char));
    if (str == NULL) {
        debug("game_unparse_move: malloc failed");
        return NULL;
    }
    str[0] = move->move;
    str[1] = '\0';
    return str;
}