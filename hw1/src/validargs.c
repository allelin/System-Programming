#include <stdlib.h>

#include "debug.h"
#include "fliki.h"
#include "global.h"

/**
 * @brief Validates command line arguments passed to the program.
 * @details This function will validate all the arguments passed to the
 * program, returning 0 if validation succeeds and -1 if validation fails.
 * Upon successful return, the various options that were specified will be
 * encoded in the global variable 'global_options', where it will be
 * accessible elsewhere in the program.  For details of the required
 * encoding, see the assignment handout.
 *
 * @param argc The number of arguments passed to the program from the CLI.
 * @param argv The argument strings passed to the program from the CLI.
 * @return 0 if validation succeeds and -1 if validation fails.
 * @modifies global variable "global_options" to contain an encoded representation
 * of the selected program options.
 * @modifies global variable "diff_filename" to point to the name of the file
 * containing the diffs to be used.
 */

int validargs(int argc, char **argv) {
    // Checks if there is more to the command line
    if (argc < 2) {
        return -1;
    }
    global_options = 0;

    // i starts at 1 because we want to read past the program name
    for (int i = 1; i < argc; i++) {
        // Getting the updated argv and first char of that argv
        char *arg = *(argv + i);
        char firstchar = *arg;

        // The if cases for when char starts with "-"
        if (firstchar == '-') {
            // Getting the second char of that argv
            char secondchar = *(arg + 1);
            char thirdchar = *(arg + 2);
            if (thirdchar != 0) {
                global_options = 0;
                return -1;
            }

            if (secondchar == 'h') {
                if (i == 1) {
                    global_options = global_options | HELP_OPTION;
                    return 0;
                } else {
                    global_options = 0;
                    return -1;
                }
            }
            else if (secondchar == 'n') {
                global_options = global_options | NO_PATCH_OPTION;
            }
            else if (secondchar == 'q') {
                global_options = global_options | QUIET_OPTION;
            } 
            else {
                global_options = 0;
                return -1;
            }
        }
        // If the file name is not the last time in the command line then it will return -1 otherwise it will save the file name
        else if (i == (argc - 1)) {
            char *last = *(argv + (argc - 1));
            diff_filename = last;
            if (diff_filename != NULL) {
                return 0;
            } else {
                global_options = 0;
                return -1;
            }
        } else {
            global_options = 0;
            return -1;
        }
    }
    global_options = 0;
    return -1;
}
