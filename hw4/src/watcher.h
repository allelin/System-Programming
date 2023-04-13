#ifndef WATCHER_H
#define WATCHER_H
// #pragma once
#include <stddef.h>

// #include "ticker.h"

enum WatcherStatus {
    // Define the watcher status
    WATCHER_RUNNING,
    WATCHER_STOPPING,
    WATCHER_STOPPED
};
typedef struct watcher {
    // Define the watcher structure
    int id;
    int pid;
    int read_pipe;
    int write_pipe;
    int tracing_enabled;
    char *arguments[10];
    char *partial_input;
    enum WatcherStatus status;
    int type_watcher;

} WATCHER;
static struct watcher **watcher_arrays;
static size_t watcher_array_size;

// quit the program gracefully
extern int quit_watchers();
#endif

// typedef struct {
//     int index;
// } ArrayElement;

// typedef struct {
//     ArrayElement *elements;
//     size_t size;
//     size_t capacity;
// } DynamicArray;

/*
EXAMPLE OF WHAT TO DO WHEN ITS DYNAMICALLY ALLOCATED
struct watcher *w = malloc(sizeof(struct watcher));
w->arguments = malloc(10 * sizeof(char *));  // allocate space for 10 strings
w->num_arguments = 0;
w->max_arguments = 10;

// Add an argument
char *arg = strdup("hello");
w->arguments[w->num_arguments++] = arg;
*/