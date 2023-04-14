#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>
#include "debug.h"
#include "ticker.h"
#include "watcher.h"
#include "bitstamp.h"


WATCHER_TYPE *bitstamp_watcher_type = &watcher_types[BITSTAMP_WATCHER_TYPE];
/*
Start(WATCHER_TYPE *type, char *args[])
i. Make sure you have at least one argument (channel to subscribe to)
ii. Create two pipes, one for child->parent and another for parent->child
iii. Fork
1. Close the parents end of the pipes
2. Use dup2 to redirect STDOUT to child->parent pipe
3. Use dup2 to redirect STDIN to parent->child pipe
4. execvp("uwsc", type->argv);
a. execvp(“uwsc wss://ws.bitstamp.net”);
iv. Parent process
1. Close child’s end of the pipes
2. Setup ASYNC IO
3. Intialize the watcher struct
a. Set ID
b. Set status to running
c. Set pid
d. Set pipe variables
e. Keep track of whether trace is on
f. Save args
4. Using the provided channel name, make a subscription
a. msg = "{"event":"bts:subscribe", "data": {"channel":
“args[0]" } }"
b. type->send(wp,msg);
*/
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

        
        if (execvp("uwsc", type->argv) == -1) {
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
        memset(wp->arguments, 0, sizeof(wp->arguments)); // clear
        debug("wp->arguments[1] = %s", wp->arguments[1]);
        int i = 0;
        // save args
        debug("args[0]: %s", args[0]);
        debug("args[1]: %s", args[1]);
        while (args[i] != NULL) {
            wp->arguments[i] = args[i];
            debug("wp->arguments[%d]: %s", i, wp->arguments[i]);
            i++;
        }
        wp->arguments[i] = NULL;
        debug("watcher_array_size: %lu", watcher_array_size);
        // using the provided channel name, make a subscription
        char *msg = NULL;
        asprintf(&msg, "{\"event\":\"bts:subscribe\", \"data\": {\"channel\": \"%s\" } }", wp->arguments[0]);
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
    if (fcntl(wp->read_pipe, F_SETFL, 0) == -1) {
        perror("Error: Failed to remove async I/O");
        exit(1);
    }
    // close the pipes
    if (close(wp->read_pipe) == -1) {
        perror("Error: Failed to close read pipe");
        exit(1);
    }
    if (close(wp->write_pipe) == -1) {
        perror("Error: Failed to close write pipe");
        exit(1);
    }
    // set the status to STOPPING
    debug("Setting status to STOPPING");
    debug("wp->status: %d", wp->status);
    wp->status = WATCHER_STOPPING;
    debug("wp->status: %d", wp->status);
    // kill the child process
    if (kill(wp->pid, SIGTERM) == -1) {
        perror("Error: Failed to kill child process");
        exit(1);
    }
    return 0;
}

int bitstamp_watcher_send(WATCHER *wp, void *arg) {
    // TO BE IMPLEMENTED
    return 1;
}

int bitstamp_watcher_recv(WATCHER *wp, char *txt) {
    // TO BE IMPLEMENTED
    abort();
}

int bitstamp_watcher_trace(WATCHER *wp, int enable) {
    // TO BE IMPLEMENTED
    abort();
}
