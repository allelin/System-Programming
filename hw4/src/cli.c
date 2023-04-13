#include "cli.h"

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
#include "store.h"
#include "ticker.h"
#include "watcher.h"

WATCHER_TYPE *cli_watcher_type = &watcher_types[CLI_WATCHER_TYPE];
volatile int first_call = 1;
// static struct watcher **watcher_arrays;
// static size_t watcher_array_size;

// Function to add to watcher array
int add_watcher(WATCHER *wp) {
    // Find the first null in the array
    size_t i = 0;
    while (i < watcher_array_size && watcher_arrays[i] != NULL) {
        i++;
    }

    // if it reached the end and it was full then we need to resize the array to fit in the watcher
    if (i == watcher_array_size) {
        watcher_array_size = watcher_array_size + 1;
        watcher_arrays = realloc(watcher_arrays, watcher_array_size * sizeof(struct watcher *));
        if (watcher_arrays == NULL) {
            debug("Error reallocating memory");
            return 1;
        }
    }
    // add the watcher to the array with index i
    watcher_arrays[i] = wp;
    // watcher_array_size += 1;
    return 0;
}

int remove_watcher(int watcher_id) {
    if (watcher_arrays[watcher_id] != NULL) {
        free(watcher_arrays[watcher_id]);
        watcher_arrays[watcher_id] = NULL;
        watcher_array_size -= 1;
        return 0;
    } else {
        debug("Watcher with id %d does not exist", watcher_id);
        return 1;
    }
}

WATCHER *cli_watcher_start(WATCHER_TYPE *type, char *args[]) {
    // IF ITS THE FIRST TIME IT IS BEING CALLED THEN WE SHOULD INITIALIZE THE ARRAY AND ALSO MAKE SURE IT CAN ONLY BE CALLED ONCE FOR CLI
    if (first_call) {
        // Initialize the array
        // Set the first_call flag to 0
        first_call = 0;
        // struct watcher **watcher_arrays;
        watcher_array_size = 1;
        watcher_arrays = malloc(watcher_array_size * sizeof(struct watcher *));
    } else {
        // Throw an error
        perror("CLI watcher can only be called once");
        exit(1);
    }

    // Allocate memory for the watcher struct
    WATCHER *wp = malloc(sizeof(WATCHER));
    // Fill in the fields of the watcher struct ID, class variables, pipe and file number for reading and writing
    if (wp == NULL) {
        perror("Failed to allocate memory for watcher");
        exit(1);
    }
    wp->id = 0;
    wp->pid = -1;
    wp->type_watcher = CLI_WATCHER_TYPE;
    wp->status = WATCHER_RUNNING;
    wp->read_pipe = STDIN_FILENO;
    wp->write_pipe = STDOUT_FILENO;
    wp->tracing_enabled = 0;
    // add the rest later

    // Add the watcher to the array
    if (add_watcher(wp) != 0) {
        perror("Failed to add watcher to array");
        exit(1);
    }
    return wp;
}

int cli_watcher_stop(WATCHER *wp) {
    // We cant really stop the CLI since its the main process unless everything is being closed aka quit
    return 1;
}

int cli_watcher_send(WATCHER *wp, void *arg) {
    // print to stdout
    char *msg = (char *)arg;
    printf("%s", msg);
    fflush(stdout);
    return 0;
}

int cli_watcher_recv(WATCHER *wp, char *txt) {
    char *current_command = txt;
    // size_t current_command_size = 0;
    int current_read_bytes = strlen(txt);
    // int number_of_commands = 0;
    int word_spot = 0;

    int space_counter = 0;
    for (int i = 0; i < current_read_bytes; i++) {
        if (current_command[i] == ' ') {
            current_command[i] = '\0';
            space_counter++;
        } else if (current_command[i] == '\n') {
            current_command[i] = '\0';
        }
    }
    debug("space_counter: %d", space_counter);
    char *words[space_counter + 2];
    char *current_word = current_command;
    // add the first word into the array
    words[word_spot] = current_word;
    debug("current_word: %s", current_word);
    debug("words[%d]: %s", word_spot, words[word_spot]);
    word_spot++;

    if (space_counter > 0) {
        // loop through the command to check for null terminators which means its the end of a word
        for (int i = 0; i < current_read_bytes; i++) {
            if (current_command[i] == '\0') {
                debug("current_word: %s", current_word);
                words[word_spot] = current_word;
                word_spot++;
                if (word_spot > space_counter + 1) {
                    break;
                }
                current_word = &current_command[i + 1];
                debug("words[%d]: %s", word_spot - 1, words[word_spot - 1]);
            }
        }
    }
    words[word_spot] = '\0';
    // Based on the number of commands we need to call each one by one using getline
    debug("space_counter: %d", space_counter);
    if (space_counter == 0) {
        // this means there is only one argument
        debug("words[0]: %s", words[0]);
        debug("%d", strcmp(words[0], "quit"));
        if (strcmp(words[0], "quit") == 0) {
            quit_watchers();
        } else if (strcmp(words[0], "watchers") == 0) {
            // print out all the watchers
            for (int i = 0; i < watcher_array_size; i++) {
                if (watcher_arrays[i] != NULL) {
                    char *buffer1 = NULL;
                    char *str = NULL;
                    int argv_length = 0;
                    if ((asprintf(&str, "%d\t%s(%d,%d,%d)", watcher_arrays[i]->id, watcher_types[watcher_arrays[i]->type_watcher].name, watcher_arrays[i]->pid, watcher_arrays[i]->read_pipe, watcher_arrays[i]->write_pipe)) < 0) {
                        perror("Error: asprintf");
                        exit(1);  // Check if its return or exit
                    }
                    cli_watcher_send(watcher_arrays[0], str);
                    debug("str: %s", str);
                    // Create a loop to find the number of arguments for the watcher
                    debug("argv_length: %d", argv_length);
                    if (!watcher_arrays[i]->type_watcher == CLI_WATCHER_TYPE) {
                        while (watcher_types[watcher_arrays[i]->type_watcher].argv[argv_length] != NULL) {
                            debug("argv_length: %d", argv_length);
                            argv_length++;
                        }
                        // Find if there are any arguments in the watcher if there is get the length of the arguments to find the total length
                        for (int j = 0; j < argv_length; j++) {
                            if ((asprintf(&buffer1, " %s", watcher_types[watcher_arrays[i]->type_watcher].argv[j])) < 0) {
                                perror("Error: asprintf");
                                exit(1);  // Check if its return or exit
                            }
                            cli_watcher_send(watcher_arrays[0], buffer1);
                        }
                    }
                    cli_watcher_send(watcher_arrays[0], "\n");
                    free(str);
                    str = NULL;
                    free(buffer1);
                    buffer1 = NULL;
                }
            }
            cli_watcher_send(watcher_arrays[0], "ticker> ");
        } else {
            perror("Invalid command");
            cli_watcher_send(wp, "???");
        }
    }
    if (space_counter == 1) {
        // this means there are two arguments
        // trace
        if (strcmp(words[0], "trace") == 0) {
            // check if the second argument is a number
            int index = atoi(words[1]);
            if (index <= watcher_array_size) {
                // check if the watcher exists
                if (watcher_arrays[index] != NULL) {
                    // check if the watcher is running
                    if (watcher_arrays[index]->status == WATCHER_RUNNING) {
                        // set the tracing flag to 1
                        watcher_arrays[index]->tracing_enabled = 1;
                    } else {
                        debug("Watcher with id %d is not running", index);
                        return 1;
                    }
                } else {
                    debug("Watcher with id %d does not exist", index);
                    return 1;
                }
            } else {
                debug("Watcher with id %d does not exist", index);
                return 1;
            }
        }
        // untrace
        else if (strcmp(words[0], "untrace") == 0) {
            // check if the second argument is a number
            int index = atoi(words[1]);
            if (index <= watcher_array_size) {
                // check if the watcher exists
                if (watcher_arrays[index] != NULL) {
                    // check if the watcher is running
                    if (watcher_arrays[index]->status == WATCHER_RUNNING) {
                        // set the tracing flag to 1
                        watcher_arrays[index]->tracing_enabled = 0;
                    } else {
                        debug("Watcher with id %d is not running", index);
                        return 1;
                    }
                } else {
                    debug("Watcher with id %d does not exist", index);
                    return 1;
                }
            } else {
                debug("Watcher with id %d does not exist", index);
                return 1;
            }
        }
        // show
    }

    // TO BE IMPLEMENTED
    // if (wp->tracing_enable) {
    //     printf("Received message: %s", txt);
    //     fflush(stdout);
    // }

    // // Parse received message into array of arguments
    // int argc = 0;
    // char *argv[10];
    // char *tok = strtok(txt, " \n");
    // while (tok != NULL && argc < 10) {
    //     argv[argc++] = tok;
    //     tok = strtok(NULL, " \n");
    // }

    // // Switch on argument count
    // switch (argc) {
    //     case 1:
    //         if (strcmp(argv[0], "quit") == 0) {
    //             // Stop all watchers
    //             for (int i = 0; i < watcher_array_size; i++) {
    //                 if (watcher_array[i] != NULL) {
    //                     stop_watcher(watcher_array[i]);
    //                 }
    //             }
    //             // Wait for all watchers to stop
    //             for (int i = 0; i < watcher_array_size; i++) {
    //                 if (watcher_array[i] != NULL) {
    //                     wait_watcher(watcher_array[i]);
    //                 }
    //             }
    //             exit(0);
    //         } else if (strcmp(argv[0], "watchers") == 0) {
    //             // Show watcher table
    //             show_watcher_table();
    //         } else {
    //             printf("Unknown command\n");
    //             fflush(stdout);
    //         }
    //         break;
    //     case 2:
    //         if (strcmp(argv[0], "trace") == 0) {
    //             // Enable tracing on specified watcher
    //             int watcher_id = atoi(argv[1]);
    //             WATCHER *w = get_watcher_by_id(watcher_id);
    //             if (w != NULL) {
    //                 w->tracing_enable = 1;
    //             }
    //         } else if (strcmp(argv[0], "untrace") == 0) {
    //             // Disable tracing on specified watcher
    //             int watcher_id = atoi(argv[1]);
    //             WATCHER *w = get_watcher_by_id(watcher_id);
    //             if (w != NULL) {
    //                 w->tracing_enable = 0;
    //             }
    //         } else if (strcmp(argv[0], "show") == 0) {
    //             // Get and print store value
    //             int watcher_id = atoi(argv[1]);
    //             WATCHER *w = get_watcher_by_id(watcher_id);
    //             if (w != NULL && w->type->get_store_value != NULL) {
    //                 char *value = w->type->get_store_value(w);
    //                 printf("Store value: %s\n", value);
    //                 fflush(stdout);
    //                 free(value);
    //             }
    //         } else if (strcmp(argv[0], "stop") == 0) {
    //             // Stop specified watcher
    //             int watcher_id = atoi(argv[1]);
    //             WATCHER *w = get_watcher_by_id(watcher_id);
    //             if (w != NULL) {
    //                 stop_watcher(w);
    //             }
    //         } else {
    //             printf("Unknown command\n");
    //             fflush(stdout);
    //         }
    //         break;
    //     default:
    //         if (strcmp(argv[0], "start") == 0) {
    //             // Start new watcher
    //             if (argc < 3) {
    //                 printf("Not enough arguments\n");
    //                 fflush(stdout);
    //             } else {
    //                 char *watcher_type = argv[1];
    //                 WATCHER_TYPE *type = get_watcher_type_by_name(watcher_type);
    //                 if (type == NULL) {
    //                     printf
    //                 }
    //             }
    //         }
    // }
    return 0;
}

int cli_watcher_trace(WATCHER *wp, int enable) {
    wp->tracing_enabled = enable;
    return 0;
}