// #define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#include "argo.h"
#include "debug.h"
#include "store.h"
#include "ticker.h"
#include "watcher.h"

#include "bitstamp.h"

WATCHER_TYPE *bitstamp_watcher_type = &watcher_types[BITSTAMP_WATCHER_TYPE];
WATCHER *bitstamp_watcher_start(WATCHER_TYPE *type, char *args[]) {
    // check if we have at least one argument (for use to subscribe to)
    debug("args[1]: %s", args[1]);
    if (args[0] == NULL) {
        perror("Error: No arguments given");
        exit(1);
    }

    // create 2 pipes for communication (child->parent and parent->child)
    int to_parent[2], to_child[2];
    if (pipe(to_parent) == -1 || pipe(to_child) == -1) {
        perror("Error: Failed to create pipe");
        exit(1);
    }

    pid_t pid = fork();
    if (pid == -1) {
        perror("Error: Failed to fork");
        exit(1);
    }
    WATCHER *wp = malloc(sizeof(WATCHER));
    if (pid == 0) {
        // child process
        // close the parent's end of the pipes
        close(to_child[1]);
        close(to_parent[0]);

        // redirect STDOUT to child->parent pipe
        if (dup2(to_child[0], STDIN_FILENO) == -1) {
            perror("Error: Failed to redirect STDOUT to child->parent pipe");
            exit(1);
        }
        // redirect STDIN to parent->child pipe
        if (dup2(to_parent[1], STDOUT_FILENO) == -1) {
            perror("Error: Failed to redirect STDIN to parent->child pipe");
            exit(1);
        }

        // execvp("uwsc", type->argv);
        // char *arg1 = watcher_types[BITSTAMP_WATCHER_TYPE].argv[0];
        // char *arg2 = watcher_types[BITSTAMP_WATCHER_TYPE].argv[1];

        // execvp(arg1, arg2);

        debug("asdfjlkasdflasjdflasjdflksdfjklasdfjlksadargs[0]: %s", type->argv[0]);
        debug("asdfjlkasdflasjdflasjdflksdfjklasdfjlksadargs[0]: %s", type->argv[0]);
        if (execvp(type->argv[0], type->argv) == -1) {
            perror("Error: Failed to execvp");
            exit(1);
        }
    } else {
        // parent process
        // close the child's end of the pipes
        close(to_child[0]);
        close(to_parent[1]);

        // setup ASYNC IO
        // int flags = fcntl(to_parent[0], F_GETFL, 0);

        if (fcntl(to_parent[0], F_SETFL, O_NONBLOCK | O_ASYNC) == -1) {
            perror("Error: Failed to setup ASYNC IO");
            exit(1);
        }
        // if (fcntl(to_parent[0], F_SETFL, flags | O_ASYNC) == -1) {
        //     perror("Error: Failed to setup ASYNC IO");
        //     exit(1);
        // }
        if (fcntl(to_parent[0], F_SETOWN, getpid()) == -1) {
            perror("Error: Failed to setup ASYNC IO");
            exit(1);
        }
        if (fcntl(to_parent[0], F_SETSIG, SIGIO) == -1) {
            perror("Error: Failed to setup ASYNC IO");
            exit(1);
        }

        // initialize the watcher struct

        if (wp == NULL) {
            perror("Error: Failed to allocate memory for watcher");
            exit(1);
        }
        int index = add_watcher(wp);
        debug("watcher_array_size: %lu", watcher_array_size);
        wp->id = index;
        debug("Watcher ID: %d", wp->id);
        wp->status = WATCHER_RUNNING;
        wp->pid = pid;
        wp->write_pipe = to_child[1];
        wp->read_pipe = to_parent[0];
        wp->tracing_enabled = 0;
        wp->type_watcher = BITSTAMP_WATCHER_TYPE;
        wp->serial_number = 0;
        // memset(wp->arguments, 0, sizeof(wp->arguments)); // clear
        // debug("wp->arguments[1] = %s", wp->arguments[1]);
        int i = 0;
        // save args
        debug("args[0]: %s", args[0]);
        debug("args[1]: %s", args[1]);
        while (args[i] != NULL) {
            i++;
        }
        wp->arguments = malloc(sizeof(char *) * (i + 1));
        int k = 0;
        while (args[k] != NULL) {
            wp->arguments[k] = strdup(args[k]);
            k++;
        }
        wp->arguments[k] = NULL;
        // wp->arguments[i] = NULL;
        debug("watcher_array_size: %lu", watcher_array_size);
        // using the provided channel name, make a subscription
        char *msg = NULL;
        asprintf(&msg, "{\"event\":\"bts:subscribe\", \"data\": {\"channel\": \"%s\" } }\n", wp->arguments[0]);
        debug("HERE");
        // type->send(wp, msg);
        debug("HERE");
        if (type->send(wp, msg) < 0) {
            perror("Error: Failed to send subscription message");
            exit(1);
        }
        free(msg);
    }
    return wp;
}
/*
Stop
i. Remove async I/O fcntl(wp->pipe_read, F_SETFL, 0);
ii. Close the pipes
iii. Set the status to STOPPING
iv. Kill the child process kill(wp->pid, SIGTERM)
*/
int bitstamp_watcher_stop(WATCHER *wp) {
    // remove async I/O
    // if (fcntl(wp->read_pipe, F_SETFL, 0) == -1) {
    //     debug("Error: Failed to remove async I/O");
    //     return 1;
    // }
    // // close the pipes
    // if (close(wp->read_pipe) == -1) {
    //     debug("Error: Failed to close read pipe");
    //     return 1;
    // }
    // if (close(wp->write_pipe) == -1) {
    //     debug("Error: Failed to close write pipe");
    //     return 1;
    // }
    // set the status to STOPPING
    debug("Setting status to STOPPING");
    debug("wp->status: %d", wp->status);
    wp->status = WATCHER_STOPPING;
    debug("wp->status: %d", wp->status);
    // kill the child process
    kill(wp->pid, SIGTERM);
    // if (kill(wp->pid, SIGTERM) == -1) {
    //     perror("Error: Failed to kill child process");
    //     exit(1);
    // }
    return 0;
}

int bitstamp_watcher_send(WATCHER *wp, void *arg) {
    // Write whatever message to the parent->child pipe
    // set arg to char *
    // char *args = (char *)arg;
    // printf("Sending: %s", args);
    if (write(wp->write_pipe, arg, strlen(arg)) < 0) {
        perror("Error: Failed to write to pipe");
        exit(1);
    }
    return 0;
}

/*
i. If tracing is enabled, trace the message
ii. Annoying string parsing
iii. Figuring out how Stark’s stupid JSON library works
iv. Store price to the argo store (replace existing value)
v. Store volume to the argo store (add to existing value)
*/
int bitstamp_watcher_recv(WATCHER *wp, char *txt) {
    debug("Received: %s", txt);
    debug("Received: %s", txt);
    debug("Received: %s", txt);
    debug("Received: %s", txt);
    debug("Received: %s", txt);
    // printf("Received: %s", txt);
    if (wp->tracing_enabled == 1) {
        wp->serial_number++;
        tracing(wp, txt);
        // printf("Tracing: %s", txt);
    }
    // loop the string to the first { and cut the last 2 characters
    size_t len = strlen(txt);
    size_t i = 0;
    int flag_no_json = 0;
    for (i = 0; i < len; i++) {
        if (txt[i] == '{') {
            break;
        }
        if (txt[i] == '\n') {
            flag_no_json = 1;
            break;
        }
    }
    if (flag_no_json == 1) {
        return 1;
    }
    char *new_txt = malloc(len - i - 2 + 1);
    strncpy(new_txt, txt + i, len - i - 2);
    new_txt[len - i - 2] = '\0';
    size_t length = strlen(new_txt);
    FILE *file = fmemopen(new_txt, length, "r");
    if (file == NULL) {
        perror("Error: Failed to open file");
        // free
        free(new_txt);
        fclose(file);
        exit(1);
    }
    ARGO_VALUE *value = argo_read_value(file);
    if (value == NULL) {
        debug("Error: Failed to read value");
        // free
        free(new_txt);
        fclose(file);
        return 1;
    }
    // get the data
    ARGO_VALUE *data = argo_value_get_member(value, "data");
    if (data == NULL) {
        debug("Error: Failed to get data");
        // free
        free(new_txt);
        fclose(file);
        argo_free_value(value);
        return 1;
    }
    ARGO_VALUE *price = argo_value_get_member(data, "price");
    if (price == NULL) {
        debug("Error: Failed to get price");
        // free
        free(new_txt);
        fclose(file);
        argo_free_value(value);
        return 1;
    }
    ARGO_VALUE *amount = argo_value_get_member(data, "amount");
    if (amount == NULL) {
        debug("Error: Failed to get amount");
        // free
        free(new_txt);
        fclose(file);
        argo_free_value(value);
        return 1;
    }
    debug("1");
    double amount_double;
    argo_value_get_double(amount, &amount_double);
    double price_double;
    argo_value_get_double(price, &price_double);
    // printf("amount: %f", amount_double);
    // printf("price: %f", price_double);
    debug("2");
    // store price and volume to the data store
    // make or get the key
    char *key1 = NULL;
    asprintf(&key1, "%s:%s:price", watcher_types[wp->type_watcher].name, wp->arguments[0]);
    struct store_value store_price = {
        .type = STORE_DOUBLE_TYPE,
        .content = {
            .double_value = price_double}};
    debug("3");
    if (store_put(key1, &store_price) == -1) {
        debug("Error: Failed to put price to store");
        // free
        free(key1);
        free(new_txt);
        fclose(file);
        argo_free_value(value);
        return 1;
    }
    free(key1);

    // get the value for volume
    char *key2 = NULL;
    asprintf(&key2, "%s:%s:volume", watcher_types[wp->type_watcher].name, wp->arguments[0]);

    // struct store_value store_volume1 = {
    //     .type = STORE_DOUBLE_TYPE,
    //     .content = {
    //         .double_value = 0.0
    //     }
    // };

    // struct store_value store_volume2 = {
    //     .type = STORE_DOUBLE_TYPE,
    //     .content = {
    //         .double_value = amount_double
    //     }
    // };
    debug("4");
    struct store_value *store_volume1 = store_get(key2);

    debug("4");
    double old_volume;
    if (store_volume1 == NULL) {
        old_volume = 0.0;
    } else {
        if (store_volume1->type != STORE_DOUBLE_TYPE) {
            debug("Error: value in store is not double");
            // free
            free(new_txt);
            free(key2);
            fclose(file);
            argo_free_value(value);
            return 1;
        }
        old_volume = store_volume1->content.double_value;
    }
    // get the double from volume1
    debug("5");
    double total = old_volume + amount_double;
    // create a struct to store the total
    struct store_value store_total = {
        .type = STORE_DOUBLE_TYPE,
        .content = {
            .double_value = total}};
    // add volume1 and volume2
    // put the total to the store
    if (store_put(key2, &store_total) == -1) {
        debug("Error: Failed to put volume to store");
        // free
        free(new_txt);
        free(key2);
        fclose(file);
        argo_free_value(value);
        return 1;
    }
    debug("6");
    if (store_volume1 != NULL) {
        store_free_value(store_volume1);
    }
    debug("6.5");
    free(key2);
    debug("7");
    // free the values
    argo_free_value(value);
    debug("8");
    // free the file
    fclose(file);
    // free new_txt
    free(new_txt);
    debug("9");
    return 0;
}

int bitstamp_watcher_trace(WATCHER *wp, int enable) {
    if (enable == 0) {
        wp->tracing_enabled = 0;
        return 0;
    } else {
        wp->tracing_enabled = 1;
        return 0;
    }
}
