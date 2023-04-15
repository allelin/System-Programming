// #define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#include "debug.h"
#include "store.h"
#include "ticker.h"
#include "watcher.h"
#include "argo.h"
#include "store.h"
#include "cli.h"
WATCHER_TYPE *cli_watcher_type = &watcher_types[CLI_WATCHER_TYPE];
volatile int first_call = 1;
size_t watcher_array_size = 0;
size_t watcher_array_capacity = 0;
struct watcher **watcher_arrays = NULL;
// static struct watcher **watcher_arrays;
// static size_t watcher_array_size;

// Function to add to watcher array
int add_watcher(WATCHER *wp) {
    // Find the first null in the array
    size_t i = 0;
    while (i < watcher_array_capacity && watcher_arrays[i] != NULL) {
        i++;
    }
    watcher_array_size++;
    // if it reached the end and it was full then we need to resize the array to fit in the watcher
    if (i == watcher_array_capacity) {
        watcher_array_capacity++;
        watcher_arrays = realloc(watcher_arrays, watcher_array_capacity * sizeof(struct watcher *));
        if (watcher_arrays == NULL) {
            debug("Error reallocating memory");
            exit(1);
        }
    }
    // add the watcher to the array with index i
    watcher_arrays[i] = wp;
    // watcher_array_size += 1;
    debug("Added watcher with id %ld", i);
    debug("watcher_array[i] = %d", watcher_arrays[i]->id);
    debug("watcher_array[i] = %d", watcher_arrays[i]->id);
    debug("watcher_array[i] = %d", watcher_arrays[i]->id);
    debug("watcher_array[i] = %d", watcher_arrays[i]->id);
    debug("watcher_array[i] = %d", watcher_arrays[i]->id);
    return i;
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

// trace function
/*
If trace is enabled, you trace the received message (write a function for this)
*/
char *get_timestamp() {
    // struct timeval tv;
    // gettimeofday(&tv, NULL);
    // snprintf(timestamp, timestamp_size, "[%ld.%06ld]", (long)tv.tv_sec, (long)tv.tv_usec);

    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    int seconds = (int)ts.tv_sec;
    int microseconds = (int)(ts.tv_nsec / 1000);
    debug("seconds: %d", seconds);
    debug("microseconds: %d", microseconds);
    // char *timestamp = (char *) malloc(sizeof(char) * 20);
    char *timestamp = NULL;
    asprintf(&timestamp, "[%010d.%06d]", seconds, microseconds);
    return timestamp;
}

void tracing(WATCHER *wp, char *message) {
    if (wp->tracing_enabled) {
        // write the message to the file
        char *timestamp = get_timestamp();
        // printf("%s\n", timestamp);
        char *combined_string = NULL;
        int check = asprintf(&combined_string, "%s[%s][ %d][   %d]: > %s", timestamp, watcher_types[wp->type_watcher].name, wp->read_pipe, wp->serial_number, message);
        // print everything to standard error output
        if (check < 0) {
            debug("Error in asprintf");
            exit(1);
        }
        fprintf(stderr, "%s\n", combined_string);
        // free
        free(timestamp);
        free(combined_string);
    }
}

WATCHER *cli_watcher_start(WATCHER_TYPE *type, char *args[]) {
    // IF ITS THE FIRST TIME IT IS BEING CALLED THEN WE SHOULD INITIALIZE THE ARRAY AND ALSO MAKE SURE IT CAN ONLY BE CALLED ONCE FOR CLI
    if (first_call || watcher_array_size == 0) {
        // Initialize the array
        // Set the first_call flag to 0
        first_call = 0;
        // struct watcher **watcher_arrays;
        // watcher_array_size = 1;
        watcher_arrays = malloc(watcher_array_size * sizeof(struct watcher *));
    } else {
        // Throw an error
        // perror("CLI watcher can only be called once");
        // exit(1);
        debug("CLI watcher can only be called once");
        return NULL;
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
    wp->serial_number = 0;
    // add the rest later

    // Add the watcher to the array
    add_watcher(wp);
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
    // check if trace is enabled
    if (wp->tracing_enabled == 1) {
        // trace the message
        // update the serial number
        wp->serial_number++;
        tracing(wp, txt);
        // return 0;
    } else {
        // update the serial number
        wp->serial_number++;
    }

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
                current_word = &current_command[i + 1];
                words[word_spot] = current_word;
                word_spot++;
                if (word_spot > space_counter) {
                    break;
                }
                debug("words[%d]: %s", word_spot - 1, words[word_spot - 1]);
            }
        }
    }
    words[word_spot] = '\0';
    // Based on the number of commands we need to call each one by one using getline
    debug("space_counter: %d", space_counter);
    debug("words[0]: %s", words[0]);

    debug("\n");
    debug("\n");
    debug("\n");
    debug("watcher_array_size: %lu", watcher_array_size);
    debug("space_counter: %d", space_counter);
    if (space_counter == 0) {
        // this means there is only one argument
        debug("words[0]: %s", words[0]);
        debug("%d", strcmp(words[0], "quit"));
        debug("watcher_arrays[0]->id: %d", watcher_arrays[0]->id);
        if (strcmp(words[0], "quit") == 0) {
            quit_watchers();
        } else if (strcmp(words[0], "watchers") == 0) {
            // print out all the watchers
            int active = 0;
            int printed = 0;
            // create a while loop to go through the array and print out all the watchers that aren't null
            while (watcher_array_size > printed) {
                debug("active: %d", active);
                debug("watcher_array_size: %lu", watcher_array_size);
                debug("printed: %d", printed);
                if (watcher_arrays[active] != NULL) {
                    char *buffer1 = NULL;
                    char *buffer2 = NULL;
                    char *str = NULL;
                    int argv_length = 0;
                    if ((asprintf(&str, "%d\t%s(%d,%d,%d)", watcher_arrays[active]->id, watcher_types[watcher_arrays[active]->type_watcher].name, watcher_arrays[active]->pid, watcher_arrays[active]->read_pipe, watcher_arrays[active]->write_pipe)) < 0) {
                        perror("Error: asprintf");
                        exit(1);  // Check if its return or exit
                    }
                    cli_watcher_send(watcher_arrays[0], str);
                    printed++;
                    debug("str: %s", str);
                    free(str);
                    if (!watcher_arrays[active]->type_watcher == CLI_WATCHER_TYPE) {
                        debug("active: %d", active);
                        while (watcher_types[watcher_arrays[active]->type_watcher].argv[argv_length] != NULL) {
                            debug("argv_length: %d", argv_length);
                            argv_length++;
                        }
                        debug("HERE");
                        // Find if there are any arguments in the watcher if there is get the length of the arguments to find the total length
                        for (int j = 0; j < argv_length; j++) {
                            if ((asprintf(&buffer1, " %s", watcher_types[watcher_arrays[active]->type_watcher].argv[j])) < 0) {
                                perror("Error: asprintf");
                                exit(1);  // Check if its return or exit
                            }
                            cli_watcher_send(watcher_arrays[0], buffer1);
                            free(buffer1);
                            buffer1 = NULL;
                        }
                        // print out the arguments of the watcher
                        for (int j = 0; watcher_arrays[active]->arguments[j] != NULL; j++) {
                            if (watcher_arrays[active]->arguments[j] == NULL) {
                                break;
                            }
                            debug("watcher_arrays[active]->arguments[j]: %s", watcher_arrays[active]->arguments[j]);
                            if ((asprintf(&buffer2, "%s", watcher_arrays[active]->arguments[j])) < 0) {
                                perror("Error: asprintf");
                                exit(1);  // Check if its return or exit
                            }
                            cli_watcher_send(watcher_arrays[0], " [");
                            cli_watcher_send(watcher_arrays[0], buffer2);
                            cli_watcher_send(watcher_arrays[0], "]");
                            debug("buffer2: %s", buffer2);
                            free(buffer2);
                            buffer2 = NULL;
                        }
                    }
                    cli_watcher_send(watcher_arrays[0], "\n");
                }
                active++;
            }
            cli_watcher_send(watcher_arrays[0], "ticker> ");
            return 0;
        } else {
            cli_watcher_send(wp, "???");
            cli_watcher_send(watcher_arrays[0], "\nticker> ");
            return 1;
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
                        cli_watcher_trace(watcher_arrays[index], 1);
                        cli_watcher_send(watcher_arrays[0], "ticker> ");
                        return 0;
                    } else {
                        debug("Watcher with id %d is not running", index);
                        cli_watcher_send(wp, "???");
                        cli_watcher_send(watcher_arrays[0], "\nticker> ");
                        return 1;
                    }
                } else {
                    debug("Watcher with id %d does not exist", index);
                    cli_watcher_send(wp, "???");
                    cli_watcher_send(watcher_arrays[0], "\nticker> ");
                    return 1;
                }
            } else {
                debug("Watcher with id %d does not exist", index);
                cli_watcher_send(wp, "???");
                cli_watcher_send(watcher_arrays[0], "\nticker> ");
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
                        cli_watcher_trace(watcher_arrays[index], 0);
                        cli_watcher_send(watcher_arrays[0], "ticker> ");
                        return 0;
                    } else {
                        debug("Watcher with id %d is not running", index);
                        cli_watcher_send(wp, "???");
                        cli_watcher_send(watcher_arrays[0], "\nticker> ");
                        return 1;
                    }
                } else {
                    debug("Watcher with id %d does not exist", index);
                    cli_watcher_send(wp, "???");
                    cli_watcher_send(watcher_arrays[0], "\nticker> ");
                    return 1;
                }
            } else {
                debug("Watcher with id %d does not exist", index);
                cli_watcher_send(wp, "???");
                cli_watcher_send(watcher_arrays[0], "\nticker> ");
                return 1;
            }
        }
        // stop
        else if ((strcmp(words[0], "stop") == 0)) {
            // check if the second argument is a number
            int index = atoi(words[1]);
            if (index == 0) {
                // perror("Error: Cannot stop the CLI");
                cli_watcher_send(wp, "???");
                cli_watcher_send(watcher_arrays[0], "\nticker> ");
                return 1;
            }
            if (index < watcher_array_size) {
                // check if the watcher exists
                if (watcher_arrays[index] != NULL) {
                    // check if the watcher is running
                    if (watcher_arrays[index]->status == WATCHER_RUNNING) {
                        int get_watcher_type = 0;
                        get_watcher_type = watcher_arrays[index]->type_watcher;
                        watcher_types[get_watcher_type].stop(watcher_arrays[index]);
                        cli_watcher_send(watcher_arrays[0], "ticker> ");
                        return 0;
                    } else {
                        debug("Watcher with id %d is not running", index);
                        cli_watcher_send(wp, "???");
                        cli_watcher_send(watcher_arrays[0], "\nticker> ");
                        return 1;
                    }
                } else {
                    debug("Watcher with id %d does not exist", index);
                    cli_watcher_send(wp, "???");
                    cli_watcher_send(watcher_arrays[0], "\nticker> ");
                    return 1;
                }
            } else {
                debug("Watcher with id %d does not exist", index);
                cli_watcher_send(wp, "???");
                cli_watcher_send(watcher_arrays[0], "\nticker> ");
                return 1;
            }
        } 
            // show
            // get stuff from the store and outputs it
        else if (strcmp(words[0], "show") == 0) {
            // second argument is the key 
            struct store_value *value_from_key = store_get(words[1]);
            debug("1");
            if (value_from_key != 0) {
                // key does exist in the store
                char *from_key = NULL;
                cli_watcher_send(wp, words[1]);
                cli_watcher_send(wp, "\t");
                debug("2");
                asprintf(&from_key, "%f", value_from_key->content.double_value);
                debug("3");
                cli_watcher_send(wp, from_key);
                debug("4");
                free(from_key);
                store_free_value(value_from_key);
                cli_watcher_send(watcher_arrays[0], "\nticker> ");
                return 0;
            } else {
                // key does not exist in the store
                cli_watcher_send(wp, "???");
                cli_watcher_send(watcher_arrays[0], "\nticker> ");
                return 1;
            }
        }
        else {
            cli_watcher_send(wp, "???");
            cli_watcher_send(watcher_arrays[0], "\nticker> ");
            return 1;
        }
        
        
    }
    if (space_counter > 1) {
        // start
        debug("space_counter: %d", space_counter);
        if (strcmp(words[0], "start") == 0) {
            debug("words[1]: %s", words[1]);
            // second argument the watcher to start
            int i = 0;
            debug("HERE");
            for (i = 0; watcher_types[i].name != 0; i++) {
                debug("space_counter: %d", space_counter);
                debug("watcher_types[i].name: %s", watcher_types[i].name);
                debug("words[1]: %s", words[1]);
                debug("%d, %d", strcmp(words[1], watcher_types[i].name), i);
                if (strcmp(words[1], watcher_types[i].name) == 0) {
                    debug("THEY ARE THE SAME");
                    // get the rest of the arguments and store it in a char array
                    char *args[space_counter - 1];
                    debug("space_counter: %d", space_counter);
                    args[0] = '\0';
                    for (int j = 0; j < space_counter - 1; j++) {
                        args[j] = words[j + 2];
                        debug("args[%d]: %s", j, args[j]);
                    }
                    args[space_counter - 1] = NULL;
                    debug("args[space_counter]: %s", args[space_counter]);
                    // call the start function of bitstamp watcher
                    watcher_types[i].start(&watcher_types[i], args);  // we could send the watcher to something later if we need that step
                    break;
                }
            }
            cli_watcher_send(watcher_arrays[0], "ticker> ");
            return 0;
        } else {
            cli_watcher_send(wp, "???");
            cli_watcher_send(watcher_arrays[0], "\nticker> ");
            return 1;
        }
    } else {
        cli_watcher_send(wp, "???");
        cli_watcher_send(watcher_arrays[0], "\nticker> ");
        return 1;
    }
    return 0;
}

int cli_watcher_trace(WATCHER *wp, int enable) {
    if (enable == 0) {
        wp->tracing_enabled = 0;
        return 0;
    } else {
        wp->tracing_enabled = 1;
        return 0;
    }
}