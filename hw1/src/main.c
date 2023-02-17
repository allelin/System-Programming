#include <stdio.h>
#include <stdlib.h>

#include "debug.h"
#include "fliki.h"
#include "global.h"

#ifdef _STRING_H
#error "Do not #include <string.h>. You will get a ZERO."
#endif

#ifdef _STRINGS_H
#error "Do not #include <strings.h>. You will get a ZERO."
#endif

#ifdef _CTYPE_H
#error "Do not #include <ctype.h>. You will get a ZERO."
#endif

int main(int argc, char **argv) {
    if(validargs(argc, argv))
        USAGE(*argv, EXIT_FAILURE);
    if(global_options == HELP_OPTION)
    {
         USAGE(*argv, EXIT_SUCCESS);
    }
    FILE *fp = fopen(diff_filename, "r");
    if (patch(stdin, stdout, fp) == -1) {
        return EXIT_FAILURE;
    } else {
        return EXIT_SUCCESS;
    }
    return EXIT_FAILURE;
}

/*
 * Just a reminder: All non-main functions should
 * be in another file not named main.c
 */
