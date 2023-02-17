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

    // HUNK hp;
    // FILE *fp = fopen("rsrc/d.diff", "r+");
    // FILE *in = fopen("rsrc/test1", "r+");
    // FILE *out = fopen("rsrc/test", "r+");
    // FILE *fp = fopen("rsrc/d.diff", "r+");
    // FILE *in = fopen("rsrc/test1", "r+");
    // FILE *out = fopen("rsrc/test", "r+");
    // hunk_next(&hp, fp);
    // int d = 0;
    // while((d = hunk_getc(&hp, fp)) != ERR && d != EOS) {
        
    // }
    // printf("\n");
    // patch(in, out, fp);
    // hunk_show(&hp, stderr);
    // patch(stdin, stdout, fp);
    // hunk_next(&hp, fp);
    // int i = 220;
    // int c = 0;
    // int e = 0;
    // while ((c = hunk_getc(&hp, fp)) != ERR && c != EOS) {
    //     printf("%c", c);
    //     i--;
    // }
    // printf("\n");
    // while (e != HUNK_MAX) {
    //     printf("%c", hunk_deletions_buffer[e]);
    //     e++;
    // }
    // printf("\n");
    // c = 0;
    // while (c != HUNK_MAX) {
    //     printf("%c", hunk_additions_buffer[c]);
    //     c++;
    // }
    // printf("123123123312");
    // printf("\n");
    // printf("\n");
    // printf("\n");
    // printf("HLLOEOEOEOAS\n");
    // printf("hp->hunk_start = %d\n", hp.old_start);
    // hunk_show(&hp, stderr);

    // hunk_next(&hp, fp);
    // c = 0;
    // e = 0;
    // while ((c = hunk_getc(&hp, fp)) != ERR && c != EOS) {
    //     printf("%c", c);
    //     i--;
    // }
    // printf("\n");
    // while (e != HUNK_MAX) {
    //     printf("%c", hunk_deletions_buffer[e]);
    //     e++;
    // }
    // printf("\n");
    // c = 0;
    // while (c != HUNK_MAX) {
    //     printf("%c", hunk_additions_buffer[c]);
    //     c++;
    // }
    // hunk_next(&hp, fp);
    // c = 0;
    // e = 0;
    // while ((c = hunk_getc(&hp, fp)) != ERR && c != EOS) {
    //     printf("%c", c);
    //     i--;
    // }
    // printf("\n");
    // while (e != HUNK_MAX) {
    //     printf("%c", hunk_deletions_buffer[e]);
    //     e++;
    // }
    // printf("\n");
    // c = 0;
    // while (c != HUNK_MAX) {
    //     printf("%c", hunk_additions_buffer[c]);
    //     c++;
    // }
    // hunk_next(&hp, fp);
    // i = 220;
    // c = 0;
    // e = 0;
    // while((c = hunk_getc(&hp, fp)) != ERR && c != EOS) {
    //     printf("%c", c);
    //     i--;
    // }
    // printf("\n");
    // while(e != HUNK_MAX) {
    //     printf("%c", hunk_deletions_buffer[e]);
    //     e++;
    // }

    // TO BE IMPLEMENTED
    // return EXIT_FAILURE;
}

/*
 * Just a reminder: All non-main functions should
 * be in another file not named main.c
 */
