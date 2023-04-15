// #define _GNU_SOURCE

#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>
#include <store.h>
#include "debug.h"
#include "watcher.h"
#include <poll.h>
#include "ticker.h"
int quit_watchers() {
    // Loop through the array and free all the watchers
    for (size_t i = 0; i < watcher_array_size; i++) {
        debug("watcher_arrays[0]->id: %d, i = %ld", watcher_arrays[0]->id, i);

        if (watcher_arrays[i] != NULL) {
            // put zero in the store
            if (i != 0) {
                char *store_key;
            asprintf(&store_key, "%s:%s:volume", watcher_types[watcher_arrays[i]->type_watcher].name, watcher_arrays[i]->arguments[0]);
            struct store_value store_value = {
                .type = STORE_DOUBLE_TYPE,
                .content = {
                    .double_value = 0.0}};
            store_put(store_key, &store_value);
            free(store_key);
            }
            // Call the stop function
            watcher_types[watcher_arrays[i]->type_watcher].stop(watcher_arrays[i]);
            debug("watcher_arrays[0]->id: %d", watcher_arrays[0]->id);
            
            // also have to free the args
            if (watcher_arrays[i]->type_watcher != CLI_WATCHER_TYPE) {
                int args_size = 0;
                while (watcher_arrays[i]->arguments[args_size] != NULL) {
                    debug("watcher_arrays[0]->id: %d", watcher_arrays[0]->id);
                    args_size++;
                }
                for (size_t j = 0; j <= args_size; j++) {
                    free(watcher_arrays[i]->arguments[j]);
                }
                free(watcher_arrays[i]->arguments);
                // free(watcher_arrays[i]);
            }
            // free(watcher_arrays[i]);
        }
    }
    // free the store


    // free(watcher_arrays);
    // Exit the program
    printf("ticker> ");
    exit(0);
    // int status = 0;
    // pid_t pid = 0;
    // while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
    //     // printf("Child %d terminated with status %d", pid, status);
    //     debug("Child %d terminated with status %d", pid, status);
    //     WATCHER *watcher = get_watcher(pid);
    //     watcher->status = WATCHER_STOPPED;
    // }
    // // loop through the array and free all the watchers
    // int indexes[watcher_array_size];
    // memset(indexes, 0, sizeof(int));
    // int counter = 0;
    // for (int i = 0; i < watcher_array_size; i++) {
    //     debug("watcher_arrays[%d]->status: %d", i, watcher_arrays[i]->status);
    //     if (watcher_arrays[i]->status == WATCHER_STOPPED) {
    //         // store the indexes of the watchers that are stopping
    //         indexes[counter] = i;
    //         counter++;
    //     }
    // }
    // debug("counter: %d", counter);
    // // remove the watchers from the list
    // for (int i = 0; i < counter; i++) {
    //     remove_watcher(indexes[i]);
    // }
    // free the cli
    // free(CLI);
    // Free the array

    // return EXIT_SUCCESS;
}

volatile sig_atomic_t signal_sigchld = 0;
volatile sig_atomic_t signal_sigio = 0;
volatile sig_atomic_t signal_first_time = 1;

WATCHER *get_watcher(pid_t pid) {
    for (size_t i = 0; i < watcher_array_size; i++) {
        debug("1");
        if (watcher_arrays[i] != NULL) {
            debug("2");
            debug("i: %ld", i);
            debug("pid: %d", pid);
            debug("watcher_array_size: %ld", watcher_array_size);
            debug("watcher_arrays[0] =  %p", watcher_arrays[0]);
            debug("watcher_arrays[1] =  %p", watcher_arrays[1]);
            debug("watcher_arrays[0]->id: %d", watcher_arrays[0]->id);
            if (watcher_arrays[i]->pid == pid) {
                debug("3");
                return watcher_arrays[i];
            }
        }
    }
    return NULL;
}
void handler_signals(int sig, siginfo_t *siginfo, void *context) {
    switch (sig) {
        case SIGIO: {
            debug("SIGIO received");
            debug("sig_info: %d", siginfo->si_fd);
            signal_sigio = 1;
            break;
        }
        case SIGINT: {
            debug("SIGINT received");
            debug("Quitting...");
            quit_watchers();
            // Make a function that exits the program gracefully
            break;
        }
        case SIGCHLD: {
            debug("SIGCHLD received");

            // Make a flag that the signal for a child getting terminated has been received
            // Then get rid of the terminated child process
            signal_sigchld = 1;

            int status = 0;
            pid_t pid = 0;
            while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
                // printf("Child %d terminated with status %d", pid, status);
                debug("ERROR: %s", strerror(errno));
                debug("Child %d terminated with status %d", pid, status);
                // check if pid is in the array
                // debug("CHECKPOINT 0");
                // if (pid == -1) {
                //     debug("PID not found");
                //     break;
                // }
                debug("CHECKPOINT 1");
                WATCHER *watcher = get_watcher(pid);
                debug("ENDS HERE");
                watcher->status = WATCHER_STOPPED;
                debug("ENDS HERE");
            }
            break;
        }
        default: {
            debug("Unknown signal received");
            break;
        }
    }
}
WATCHER *CLI;
int ticker(void) {
    // volatile sig_atomic_t pid;
    struct sigaction sa;
    sa.sa_sigaction = handler_signals;
    sa.sa_flags = SA_SIGINFO;

    if (sigemptyset(&sa.sa_mask) == -1) {
        perror("Error: cannot set signal mask");
        exit(1);
    }

    if (sigaction(SIGIO, &sa, NULL) == -1) {
        perror("Error: cannot handle SIGIO");
        exit(1);
    }

    if (sigaction(SIGINT, &sa, NULL) == -1) {
        perror("Error: cannot handle SIGINT");
        exit(1);
    }

    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        perror("Error: cannot handle SIGCHLD");
        exit(1);
    }
    printf("ticker> ");
    fflush(stdout);
    if (fcntl(STDIN_FILENO, F_SETFL, O_ASYNC | O_NONBLOCK) == -1) {
        perror("Error: cannot set non-blocking mode for stdin");
        exit(1);
    }
    if (fcntl(STDIN_FILENO, F_SETOWN, getpid()) == -1) {
        perror("Error: cannot set process owner for stdin");
        exit(1);
    }
    if (fcntl(STDIN_FILENO, F_SETSIG, SIGIO) == -1) {
        perror("Error: cannot set signal for stdin");
        exit(1);
    }

    if (fcntl(STDOUT_FILENO, F_SETFL, O_ASYNC | O_NONBLOCK) == -1) {
        perror("Error: cannot set non-blocking mode for stdout");
        exit(1);
    }
    if (fcntl(STDOUT_FILENO, F_SETOWN, getpid()) == -1) {
        perror("Error: cannot set process owner for stdout");
        exit(1);
    }
    if (fcntl(STDOUT_FILENO, F_SETSIG, SIGIO) == -1) {
        perror("Error: cannot set signal for stdout");
        exit(1);
    }
    CLI = watcher_types[CLI_WATCHER_TYPE].start(&watcher_types[CLI_WATCHER_TYPE], watcher_types[CLI_WATCHER_TYPE].argv);
    // kill(getpid(), SIGIO);
    // kill(getpid(), SIGCHLD);
    while (1) {
        if (signal_sigio == 1 || signal_first_time == 1) {
            debug("signal_sigio: %d", signal_sigio);
            signal_sigio = 0;
            if (feof(stdin)) {
                debug("feof(stdin) != 0");
                exit(EXIT_SUCCESS);
            }   
            if (signal_first_time == 1) {
                signal_first_time = 0;
                // initalize CLI watcher calling start_watcher
                // CLI = watcher_types[CLI_WATCHER_TYPE].start(&watcher_types[CLI_WATCHER_TYPE], watcher_types[CLI_WATCHER_TYPE].argv);
                debug("watcher_arrays[0] =  %p", watcher_arrays[0]);
            }

            // LOOP THROUGH THE WATCHERS AND CHECK IF WE GET AN INPUT
            signal_first_time = 0;
            int w = 0;
            for (w = 0; w < watcher_array_size; w++) {
                WATCHER *watcher = watcher_arrays[w];
                if (watcher == NULL) {
                    continue;
                }

                struct pollfd fds[1];
                fds[0].fd = watcher->read_pipe;
                fds[0].events = POLLIN;
                int ret = poll(fds, 1, 0);
                if (ret == -1) {
                    perror("Error: poll");
                    exit(EXIT_FAILURE);
                }
                if (ret == 0) {
                    continue;
                }

                int fd = watcher->read_pipe;
                // debug("fd: %d", fd);

                // int available = 0;
                // if (ioctl(fd, FIONREAD, &available) < 0) {
                //     // perror("Error: cannot get number of bytes available to read");
                //     continue;
                // }
                // debug("available: %d", available);
                // debug("available: %d", available);
                // debug("available: %d", available);
                // debug("w = %d", w);
                // if (available == 0) {
                //     // if (feof(stdin)) {
                //     //     debug("feof(stdin) != 0");
                //     //     exit(EXIT_SUCCESS));
                //     // }
                //     // debug("available == 0");
                //     // int read_bytes = 0;
                //     // char buffer[1] = {0};
                //     // read_bytes = read(fd, &buffer, 1);
                //     // debug("read_bytes: %d", read_bytes);
                //     // if (read_bytes == 0) {
                //     //     quit_watchers();
                //     // }
                //     continue;
                // }
                debug("fds[0].revents: %d", fds[0].revents);
                debug("POLLIN: %d", POLLIN);
                if (fds[0].revents) {
                    debug("fds[0].revents & POLLIN");
                    char *command = NULL;
                    size_t command_size = 0;
                    debug("IN HERE");
                    FILE *stream = open_memstream(&command, &command_size);  // checked for free and fclose
                    if (stream == NULL) {
                        perror("Error opening memstream");
                        exit(1);
                    }
                    char buffer[1] = {0};
                    int read_bytes = 0;
                    char *current_command = NULL;
                    size_t current_command_size = 0;
                    ssize_t current_read_bytes = 0;
                    int number_of_commands = 0;
                    int quit_flag = 0;
                    // int word_spot = 0;
                    while (1) {
                        read_bytes = read(fd, &buffer, 1);
                        if (read_bytes == -1) {
                            if (errno == EWOULDBLOCK) {
                                break;
                            }
                        }
                        if (read_bytes == 0) {
                            debug("read_bytes == 0");
                            // printf("This gets called\n");
                            // Call quit function here
                            // quit_watchers();
                            quit_flag = 1;
                            // debug("quit_flag: %d", quit_flag);
                            // exit(EXIT_SUCCESS);
                            break;
                        }
                        fwrite(buffer, 1, read_bytes, stream);
                    }
                    debug("read_bytes: %d", read_bytes);
                    debug("Error %s \n", strerror(errno));
                    fflush(stream);
                    char *read_command = malloc(command_size + 1);  // checked for free
                    if (read_command == NULL) {
                        perror("Error: malloc");
                        return 1;
                    }
                    memcpy(read_command, command, command_size);

                    // Make a loop where it counts how many commands there are based on how many \n there are
                    for (int i = 0; i < command_size; i++) {
                        if (read_command[i] == '\n') {
                            number_of_commands++;
                        }
                    }
                    debug("Number of commands: %d", number_of_commands);

                    // Based on the number of commands we need to call each one by one using getline
                    while (number_of_commands > 0) {
                        current_read_bytes = getline(&current_command, &current_command_size, stream);  // checked for free
                        if (current_read_bytes == -1) {
                            debug("Error: getline");
                            break;
                        }
                        if (watcher_types[watcher_arrays[w]->type_watcher].recv(watcher, current_command) == 1) {
                            // watcher_types[CLI->type_watcher].send(watcher_arrays[0], "???");
                            // watcher_types[CLI->type_watcher].send(watcher_arrays[0], "\nticker> ");
                        } else {
                            // watcher_types[CLI->type_watcher].send(watcher_arrays[0], "ticker> ");
                        }
                        free(current_command);
                        current_command = NULL;
                        debug("current_read_bytes: %ld", current_read_bytes);
                        debug("THIS ONE?");
                        number_of_commands--;
                    }

                    // Setting the last \n to \0
                    // read_command[command_size - 1] = '\0';
                    if (stream != NULL) {
                        fclose(stream);
                    }
                    if (command != NULL) {
                        free(command);
                    }
                    command = NULL;
                    debug("Command: %s", read_command);
                    free(read_command);
                    read_command = NULL;
                    if (quit_flag == 1) {
                        quit_watchers();
                        break;
                    }
                }
            }

            // char *command = NULL;
            // size_t command_size = 0;
            // FILE *stream = open_memstream(&command, &command_size);  // checked for free and fclose
            // if (stream == NULL) {
            //     perror("Error opening memstream");
            //     exit(1);
            // }
            // char buffer[1] = {0};
            // int read_bytes = 0;
            // char *current_command = NULL;
            // size_t current_command_size = 0;
            // ssize_t current_read_bytes = 0;
            // int number_of_commands = 0;
            // int quit_flag = 0;
            // // int word_spot = 0;
            // while (1) {
            //     read_bytes = read(STDIN_FILENO, &buffer, 1);
            //     if (read_bytes == -1) {
            //         if (errno == EWOULDBLOCK) {
            //             break;
            //         }
            //     }
            //     if (read_bytes == 0) {
            //         // printf("This gets called\n");
            //         // Call quit function here
            //         // quit_watchers();
            //         quit_flag = 1;
            //         break;
            //     }
            //     fwrite(buffer, 1, read_bytes, stream);
            // }
            // debug("read_bytes: %d", read_bytes);
            // fflush(stream);
            // char *read_command = malloc(command_size + 1);  // checked for free
            // if (read_command == NULL) {
            //     perror("Error: malloc");
            //     return 1;
            // }
            // memcpy(read_command, command, command_size);

            // // Make a loop where it counts how many commands there are based on how many \n there are
            // for (int i = 0; i < command_size; i++) {
            //     if (read_command[i] == '\n') {
            //         number_of_commands++;
            //     }
            // }
            // debug("Number of commands: %d", number_of_commands);

            // // Based on the number of commands we need to call each one by one using getline
            // while (number_of_commands > 0) {
            //     current_read_bytes = getline(&current_command, &current_command_size, stream);  // checked for free
            //     if (current_read_bytes == -1) {
            //         debug("Error: getline");
            //         break;
            //     }
            //     if (watcher_types[CLI->type_watcher].recv(CLI, current_command) == 1) {
            //         // watcher_types[CLI->type_watcher].send(watcher_arrays[0], "???");
            //         // watcher_types[CLI->type_watcher].send(watcher_arrays[0], "\nticker> ");
            //     } else {
            //         // watcher_types[CLI->type_watcher].send(watcher_arrays[0], "ticker> ");
            //     }
            //     free(current_command);
            //     current_command = NULL;
            //     debug("current_read_bytes: %ld", current_read_bytes);
            //     debug("THIS ONE?");
            //     number_of_commands--;
            // }

            // // Setting the last \n to \0
            // // read_command[command_size - 1] = '\0';
            // if (stream != NULL) {
            //     fclose(stream);
            // }
            // // fclose(stream);
            // if (command != NULL) {
            //     free(command);
            // }
            // // free(command);
            // command = NULL;
            // debug("Command: %s", read_command);
            // fflush(stdout);

            // free(read_command);
            // read_command = NULL;
            // if (quit_flag == 1) {
            //     quit_watchers();
            //     break;
            // }
        }

        // if signal_sigchld == 1
        /*
        Searched through the list for any watchers which were marked as
        STOPPED, then removed them from the watcher list
        */
        if (signal_sigchld == 1) {
            debug("SIGCHLD");
            signal_sigchld = 0;
            int indexes[watcher_array_size];
            memset(indexes, 0, sizeof(int));
            int counter = 0;
            for (int i = 0; i < watcher_array_size; i++) {
                debug("watcher_arrays[%d]->status: %d", i, watcher_arrays[i]->status);
                if (watcher_arrays[i]->status == WATCHER_STOPPED) {
                    // store the indexes of the watchers that are stopping
                    indexes[counter] = i;
                    counter++;
                }
            }
            debug("counter: %d", counter);
            // remove the watchers from the list
            for (int i = 0; i < counter; i++) {
                remove_watcher(indexes[i]);
            }
        }
        // loop ends here
        sigset_t mask;
        sigemptyset(&mask);
        sigsuspend(&mask);
        // printf("ticker> ");
        // fflush(stdout);
    }
    return 0;
}
