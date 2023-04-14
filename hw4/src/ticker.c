#include "ticker.h"

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
#include "watcher.h"

int quit_watchers() {
    // Loop through the array and free all the watchers
    for (size_t i = 0; i < watcher_array_size; i++) {
        if (watcher_arrays[i] != NULL) {
            // Call the stop function
            watcher_types[watcher_arrays[i]->type_watcher].stop(watcher_arrays[i]);
            free(watcher_arrays[i]);
        }
    }
    // free the cli
    // free(CLI);
    // Free the array
    free(watcher_arrays);
    // Exit the program
    printf("ticker> ");
    exit(0);
    // return EXIT_SUCCESS;
}

volatile sig_atomic_t signal_sigchld = 0;
volatile sig_atomic_t signal_sigio = 0;
volatile sig_atomic_t signal_first_time = 1;

WATCHER *get_watcher(pid_t pid) {
    for (size_t i = 0; i < watcher_array_size; i++) {
        if (watcher_arrays[i] != NULL) {
            if (watcher_arrays[i]->pid == pid) {
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

            int status;
            pid_t pid;
            while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
                // printf("Child %d terminated with status %d", pid, status);
                debug("Child %d terminated with status %d", pid, status);
                WATCHER *watcher = get_watcher(pid);
                watcher->status = WATCHER_STOPPED;
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
    sigemptyset(&sa.sa_mask);

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

    // kill(getpid(), SIGIO);
    // kill(getpid(), SIGCHLD);
    while (1) {
        if (signal_sigio == 1 || signal_first_time == 1) {
            signal_sigio = 0;

            if (signal_first_time == 1) {
                signal_first_time = 0;
                // initalize CLI watcher calling start_watcher
                CLI = watcher_types[CLI_WATCHER_TYPE].start(&watcher_types[CLI_WATCHER_TYPE], watcher_types[CLI_WATCHER_TYPE].argv);
            }
            signal_first_time = 0;
            char *command = NULL;
            size_t command_size = 0;
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
                read_bytes = read(STDIN_FILENO, &buffer, 1);
                if (read_bytes == -1) {
                    if (errno == EWOULDBLOCK) {
                        break;
                    }
                }
                if (read_bytes == 0) {
                    // printf("This gets called\n");
                    // Call quit function here
                    // quit_watchers();
                    quit_flag = 1;
                    break;
                }
                fwrite(buffer, 1, read_bytes, stream);
            }
            debug("read_bytes: %d", read_bytes);
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
                watcher_types[CLI->type_watcher].recv(CLI, current_command);
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
            // fclose(stream);
            if (command != NULL) {
                free(command);
            }
            // free(command);
            command = NULL;
            debug("Command: %s", read_command);
            fflush(stdout);

            free(read_command);
            read_command = NULL;
            if (quit_flag == 1) {
                quit_watchers();
                break;
            }
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

        sigset_t mask;
        sigemptyset(&mask);
        sigsuspend(&mask);
        // printf("ticker> ");
        // fflush(stdout);
    }
    return 0;
}
