#include "fliki.h"

#include <stdio.h>
#include <stdlib.h>

#include "debug.h"
#include "global.h"

/**
 * @brief Get the header of the next hunk in a diff file.
 * @details This function advances to the beginning of the next hunk
 * in a diff file, reads and parses the header line of the hunk,
 * and initializes a HUNK structure with the result.
 *
 * @param hp  Pointer to a HUNK structure provided by the caller.
 * Information about the next hunk will be stored in this structure.
 * @param in  Input stream from which hunks are being read.
 * @return 0  if the next hunk was successfully located and parsed,
 * EOF if end-of-file was encountered or there was an error reading
 * from the input stream, or ERR if the data in the input stream
 * could not be properly interpreted as a hunk.
 */
static int counter = 0;

// GLOBAL VARIABLE TO CHECK HUNK HAS BEEN READ FROM NEXT
static int header_read = 0;
// GLOBAL VARIABLE TO CHECK IF HUNK LINE IS VALID
static int hunk_line = 0;
static int add_count = 0;
static int del_count = 0;
static int valid = 0;

static int char_counter = 0;
static int part1 = 0;
static int part2 = 0;
static int part3 = 0;
static char *original = hunk_additions_buffer;
static char *original2 = hunk_deletions_buffer;
static char *pointer = hunk_additions_buffer;
static char *letter = hunk_additions_buffer + 2;
static char *pointer2 = hunk_deletions_buffer;
static char *letter2 = hunk_deletions_buffer + 2;

static void reset(HUNK *hp) {
    hp->old_start = 0;
    hp->old_end = 0;
    hp->new_start = 0;
    hp->new_end = 0;
    hp->type = HUNK_NO_TYPE;
    int flag_old = 0;
    int flag_new = 0;
    int flag_type = 0;
    int old_comma = 0;
    int new_comma = 0;
}
int hunk_next(HUNK *hp, FILE *in) {
    // TO BE IMPLEMENTED
    /**
     * WHAT NEEDS TO BE DONE
     * CHECK IF FILE IS EMPTY AKA CHECK IF ITS AT END OF FILE
     * CHECK THE HUNK HEADERS TO SEE IF THE HUNKS ARE VALID DONE BY CHECKING OLD RANGE, ADC AND NEW RANGE
     * UPDATE THE HUNK WITH THE NEW INFORMATION
     *
     * TBD MAYBE CHECK THE BODY OF THE HUNK IN ORDER TO CHECK IF THATS A VALID BODY FOR THE HUNK HEADER HOWEVER THIS
     * CAN BE DONE IN HUNK_GET????
     * EVERY CASE AFTER THE FIRST ONE CHECK IF THERES A NEW LINE BEFORE A HEADER AKA STARTS WITH A NUMBER
     * FIND A WAY TO CHECK IF ITS THE
     *
     *
     * MIGHT HAVE TO MAKE A MACRO WHENEVER IT READS EOF DEPENDING ON THE TEST CASES
     *
     * NEED TO RESET BUFFER AFTER EVERY NEXT
     * NEED TO CHANGE HOW WE USE UNGET SINCE WE DO IT T
     */

    // If file is empty return EOF
    char read = fgetc(in);
    if (read == EOF) {
        return EOF;
    }
    pointer = original;
    pointer2 = original2;
    int i = 0;

    while (i < HUNK_MAX) {
        *pointer = 0;
        *pointer2 = 0;
        pointer++;
        pointer2++;
        i++;
    }
    pointer = original;
    pointer2 = original2;
    letter = original + 2;
    letter2 = original2 + 2;
    // Check if the hunk header is valid for the first hunk
    // So it reads the first char and checks if its a number if it is then it checks the next char to see if its a comma
    // if it is a comma it checks for a number and then a letter but if its not a comma it checks for a letter amd assigns old to new number
    // if its a letter it checks for a number and then a comma but if its not a comma then it just assigns the start to the end number
    hp->old_start = 0;
    hp->old_end = 0;
    hp->new_start = 0;
    hp->new_end = 0;
    hp->type = HUNK_NO_TYPE;
    int flag_old = 0;
    int flag_new = 0;
    int flag_type = 0;
    int old_comma = 0;
    int new_comma = 0;
    if (counter == 0) {
        // Checking the number and adding it to the old start then the old end depending on if its a comma or not
        while (read >= '0' && read <= '9') {
            hp->old_start = hp->old_start * 10 + read - '0';
            flag_old = 1;
            read = fgetc(in);
            if (read == EOF) {
                return ERR;
            }  // CHECK IF THIS IS RIGHT
        }
        if (read == ',' && flag_old == 1) {
            read = fgetc(in);
            if (read == EOF) {
                return ERR;
            }  // CHECK IF THIS IS RIGHT
            old_comma = 1;
            while (read >= '0' && read <= '9') {
                hp->old_end = hp->old_end * 10 + read - '0';
                read = fgetc(in);
                if (read == EOF) {
                    return ERR;
                }  // CHECK IF THIS IS RIGHT
            }
        } else {
            hp->old_end = hp->old_start;
        }

        // Checking if the hunk header is valid else return ERR
        if (flag_old == 0) {
            reset(hp);
            return ERR;
        }
        if (read == 'a' && flag_type == 0) {
            if (old_comma == 1) {
                reset(hp);
                return ERR;
            }
            hp->type = HUNK_APPEND_TYPE;
            flag_type = 1;
            read = fgetc(in);
            if (read == EOF) {
                return ERR;
            }  // CHECK IF THIS IS RIGHT
        }
        if (read == 'c' && flag_type == 0) {
            hp->type = HUNK_CHANGE_TYPE;
            flag_type = 1;
            read = fgetc(in);
            if (read == EOF) {
                return ERR;
            }  // CHECK IF THIS IS RIGHT
        }
        if (read == 'd' && flag_type == 0) {
            hp->type = HUNK_DELETE_TYPE;
            flag_type = 1;
            read = fgetc(in);
            if (read == EOF) {
                return ERR;
            }  // CHECK IF THIS IS RIGHT
        }
        if (hp->type == HUNK_NO_TYPE) {
            reset(hp);
            return ERR;
        }
        // Checking the number and adding it to the new start then the new end depending on if its a comma or not
        // Update the serial number and unget the last char if theres no comma
        while (read >= '0' && read <= '9') {
            hp->new_start = hp->new_start * 10 + read - '0';
            flag_new = 1;
            read = fgetc(in);
            if (read == EOF) {
                return ERR;
            }  // CHECK IF THIS IS RIGHT SINCE IT CAN BE AT THE END OF THE FILE AFTER THE HEADER
        }
        if (read == ',' && flag_new == 1) {
            read = fgetc(in);
            if (read == EOF) {
                return ERR;
            }  // CHECK IF THIS IS RIGHT SINCE IT CAN BE AT THE END OF THE FILE AFTER THE HEADER
            new_comma = 1;
            if (hp->type == HUNK_DELETE_TYPE && new_comma == 1) {
                reset(hp);
                return ERR;
            }
            while (read >= '0' && read <= '9') {
                hp->new_end = hp->new_end * 10 + read - '0';
                read = fgetc(in);
                if (read == EOF) {
                    return ERR;
                }  // CHECK IF THIS IS RIGHT SINCE IT CAN BE AT THE END OF THE FILE AFTER THE HEADER
            }
            if (hp->old_end < hp->old_start || hp->new_end < hp->new_start) {
                reset(hp);
                return ERR;
            }
            if (read == '\n') {
                ungetc(read, in);
            }
            counter++;
            hp->serial = counter;
            ////printf("Here is the serial number: %d , Here is the old start: %d , Here is the old end: %d , Here is the new start: %d , Here is the new end: %d", hp->serial, hp->old_start, hp->old_end, hp->new_start, hp->new_end);
            header_read = 1;
            return 0;
        } else {
            hp->new_end = hp->new_start;
        }
        if (read == '\n') {
            ////printf("Here");
            ungetc(read, in);
            if (hp->old_end < hp->old_start || hp->new_end < hp->new_start) {
                reset(hp);
                return ERR;
            }
            counter++;
            hp->serial = counter;
            hp->new_end = hp->new_start;
            ////printf("Here is the serial number: %d , Here is the old start: %d , Here is the old end: %d , Here is the new start: %d , Here is the new end: %d", hp->serial, hp->old_start, hp->old_end, hp->new_start, hp->new_end);
            header_read = 1;
            return 0;
        } else {
            reset(hp);
            return ERR;
        }
    }
    reset(hp);
    // printf("second call");
    //  Check if the hunk header is valid for the rest of the hunks
    while (read != EOF) {
        int flag = 0;
        if (valid == 1) {
            valid = 0;
            while (read >= '0' && read <= '9') {
                hp->old_start = hp->old_start * 10 + read - '0';
                flag_old = 1;
                read = fgetc(in);
                if (read == EOF) {
                    return ERR;
                }  // CHECK IF THIS IS RIGHT
            }
            if (read == ',' && flag_old == 1) {
                read = fgetc(in);
                while (read >= '0' && read <= '9') {
                    hp->old_end = hp->old_end * 10 + read - '0';
                    read = fgetc(in);
                    if (read == EOF) {
                        return ERR;
                    }  // CHECK IF THIS IS RIGHT
                }
            } else {
                hp->old_end = hp->old_start;
            }

            // Checking if the hunk header is valid else return ERR
            if (flag_old == 0) {
                // printf("1");
                return ERR;
            }
            if (read == 'a' && flag_type == 0) {
                hp->type = HUNK_APPEND_TYPE;
                flag_type = 1;
                read = fgetc(in);
                if (read == EOF) {
                    return ERR;
                }  // CHECK IF THIS IS RIGHT
            }
            if (read == 'c' && flag_type == 0) {
                hp->type = HUNK_CHANGE_TYPE;
                flag_type = 1;
                read = fgetc(in);
                if (read == EOF) {
                    return ERR;
                }  // CHECK IF THIS IS RIGHT
            }
            if (read == 'd' && flag_type == 0) {
                hp->type = HUNK_DELETE_TYPE;
                flag_type = 1;
                read = fgetc(in);
                if (read == EOF) {
                    return ERR;
                }  // CHECK IF THIS IS RIGHT
            }
            if (hp->type == HUNK_NO_TYPE) {
                return ERR;
            }
            // Checking the number and adding it to the new start then the new end depending on if its a comma or not
            // Update the serial number and unget the last char if theres no comma
            while (read >= '0' && read <= '9') {
                hp->new_start = hp->new_start * 10 + read - '0';
                flag_new = 1;
                read = fgetc(in);
                if (read == EOF) {
                    return ERR;
                }  // CHECK IF THIS IS RIGHT SINCE IT CAN BE AT THE END OF THE FILE AFTER THE HEADER
            }
            if (read == ',' && flag_new == 1) {
                read = fgetc(in);
                if (read == EOF) {
                    return ERR;
                }  // CHECK IF THIS IS RIGHT SINCE IT CAN BE AT THE END OF THE FILE AFTER THE HEADER
                while (read >= '0' && read <= '9') {
                    hp->new_end = hp->new_end * 10 + read - '0';
                    read = fgetc(in);
                    if (read == EOF) {
                        return ERR;
                    }  // CHECK IF THIS IS RIGHT SINCE IT CAN BE AT THE END OF THE FILE AFTER THE HEADER
                }
                if (hp->old_end < hp->old_start || hp->new_end < hp->new_start) {
                    return ERR;
                }
                if (read == '\n') {
                    ungetc(read, in);
                }
                counter++;
                hp->serial = counter;
                part1 = 0;
                part2 = 0;
                del_count = 0;
                add_count = 0;
                // printf("Here is the serial number: %d , Here is the old start: %d , Here is the old end: %d , Here is the new start: %d , Here is the new end: %d", hp->serial, hp->old_start, hp->old_end, hp->new_start, hp->new_end);
                header_read = 1;
                int i = 0;
                pointer = original;
                pointer2 = original2;
                while (i < HUNK_MAX) {
                    *pointer = 0;
                    *pointer2 = 0;
                    pointer++;
                    pointer2++;
                    i++;
                }
                pointer = original;
                pointer2 = original2;
                letter = original + 2;
                letter2 = original2 + 2;
                return 0;
            } else {
                hp->new_end = hp->new_start;
            }
            if (read == '\n') {
                ungetc(read, in);
                if (hp->old_end < hp->old_start || hp->new_end < hp->new_start) {
                    return ERR;
                }
                counter++;
                hp->serial = counter;
                hp->new_end = hp->new_start;
                part1 = 0;
                part2 = 0;
                del_count = 0;
                add_count = 0;
                // printf("Here is the serial number: %d , Here is the old start: %d , Here is the old end: %d , Here is the new start: %d , Here is the new end: %d", hp->serial, hp->old_start, hp->old_end, hp->new_start, hp->new_end);
                header_read = 1;
                int i = 0;
                pointer = original;
                pointer2 = original2;
                while (i < HUNK_MAX) {
                    *pointer = 0;
                    *pointer2 = 0;
                    pointer++;
                    pointer2++;
                    i++;
                }
                pointer = original;
                pointer2 = original2;
                letter = original + 2;
                letter2 = original2 + 2;
                return 0;
            } else {
                return ERR;
            }
        }
        read = fgetc(in);
        if (read == '\n') {
            read = fgetc(in);
            if (read != '<' && read != '>' && read != '-' && (read < '0') && (read > '9')) {
                return ERR;
            }
            switch (read) {
                case ('<'):
                    read = fgetc(in);
                    if (read != ' ') {
                        return ERR;
                    }
                    flag = 1;
                    break;
                case ('>'):
                    read = fgetc(in);
                    if (read != ' ') {
                        return ERR;
                    }
                    flag = 1;
                    break;
                case ('-'):
                    read = fgetc(in);
                    if (read != '-') {
                        return ERR;
                    }
                    read = fgetc(in);
                    if (read != '-') {
                        return ERR;
                    }
                    flag = 1;
                    break;
                default:
                    break;
            }
            if (flag == 0) {
                while (read >= '0' && read <= '9') {
                    hp->old_start = hp->old_start * 10 + read - '0';
                    flag_old = 1;
                    read = fgetc(in);
                    if (read == EOF) {
                        return ERR;
                    }  // CHECK IF THIS IS RIGHT
                }
                if (read == ',' && flag_old == 1) {
                    read = fgetc(in);
                    while (read >= '0' && read <= '9') {
                        hp->old_end = hp->old_end * 10 + read - '0';
                        read = fgetc(in);
                        if (read == EOF) {
                            return ERR;
                        }  // CHECK IF THIS IS RIGHT
                    }
                } else {
                    hp->old_end = hp->old_start;
                }

                // Checking if the hunk header is valid else return ERR
                if (flag_old == 0) {
                    // printf("1");
                    return ERR;
                }
                if (read == 'a' && flag_type == 0) {
                    hp->type = HUNK_APPEND_TYPE;
                    flag_type = 1;
                    read = fgetc(in);
                    if (read == EOF) {
                        return ERR;
                    }  // CHECK IF THIS IS RIGHT
                }
                if (read == 'c' && flag_type == 0) {
                    hp->type = HUNK_CHANGE_TYPE;
                    flag_type = 1;
                    read = fgetc(in);
                    if (read == EOF) {
                        return ERR;
                    }  // CHECK IF THIS IS RIGHT
                }
                if (read == 'd' && flag_type == 0) {
                    hp->type = HUNK_DELETE_TYPE;
                    flag_type = 1;
                    read = fgetc(in);
                    if (read == EOF) {
                        return ERR;
                    }  // CHECK IF THIS IS RIGHT
                }
                if (hp->type == HUNK_NO_TYPE) {
                    return ERR;
                }
                // Checking the number and adding it to the new start then the new end depending on if its a comma or not
                // Update the serial number and unget the last char if theres no comma
                while (read >= '0' && read <= '9') {
                    hp->new_start = hp->new_start * 10 + read - '0';
                    flag_new = 1;
                    read = fgetc(in);
                    if (read == EOF) {
                        return ERR;
                    }  // CHECK IF THIS IS RIGHT SINCE IT CAN BE AT THE END OF THE FILE AFTER THE HEADER
                }
                if (read == ',' && flag_new == 1) {
                    read = fgetc(in);
                    if (read == EOF) {
                        return ERR;
                    }  // CHECK IF THIS IS RIGHT SINCE IT CAN BE AT THE END OF THE FILE AFTER THE HEADER
                    while (read >= '0' && read <= '9') {
                        hp->new_end = hp->new_end * 10 + read - '0';
                        read = fgetc(in);
                        if (read == EOF) {
                            return ERR;
                        }  // CHECK IF THIS IS RIGHT SINCE IT CAN BE AT THE END OF THE FILE AFTER THE HEADER
                    }
                    if (hp->old_end < hp->old_start || hp->new_end < hp->new_start) {
                        return ERR;
                    }
                    if (read == '\n') {
                        ungetc(read, in);
                    }
                    counter++;
                    hp->serial = counter;
                    part1 = 0;
                    part2 = 0;
                    del_count = 0;
                    add_count = 0;
                    // printf("Here is the serial number: %d , Here is the old start: %d , Here is the old end: %d , Here is the new start: %d , Here is the new end: %d", hp->serial, hp->old_start, hp->old_end, hp->new_start, hp->new_end);
                    header_read = 1;
                    int i = 0;
                    pointer = original;
                    pointer2 = original2;
                    while (i < HUNK_MAX) {
                        *pointer = 0;
                        *pointer2 = 0;
                        pointer++;
                        pointer2++;
                        i++;
                    }
                    pointer = original;
                    pointer2 = original2;
                    letter = original + 2;
                    letter2 = original2 + 2;
                    return 0;
                } else {
                    hp->new_end = hp->new_start;
                }
                if (read == '\n') {
                    ungetc(read, in);
                    if (hp->old_end < hp->old_start || hp->new_end < hp->new_start) {
                        return ERR;
                    }
                    counter++;
                    hp->serial = counter;
                    hp->new_end = hp->new_start;
                    part1 = 0;
                    part2 = 0;
                    del_count = 0;
                    add_count = 0;
                    // printf("Here is the serial number: %d , Here is the old start: %d , Here is the old end: %d , Here is the new start: %d , Here is the new end: %d", hp->serial, hp->old_start, hp->old_end, hp->new_start, hp->new_end);
                    header_read = 1;
                    int i = 0;
                    pointer = original;
                    pointer2 = original2;
                    while (i < HUNK_MAX) {
                        *pointer = 0;
                        *pointer2 = 0;
                        pointer++;
                        pointer2++;
                        i++;
                    }
                    pointer = original;
                    pointer2 = original2;
                    letter = original + 2;
                    letter2 = original2 + 2;
                    return 0;
                } else {
                    return ERR;
                }
            }
        }
    }
    return EOF;
}

/**
 * @brief  Get the next character from the data portion of the hunk.
 * @details  This function gets the next character from the data
 * portion of a hunk.  The data portion of a hunk consists of one
 * or both of a deletions section and an additions section,
 * depending on the hunk type (delete, append, or change).
 * Within each section is a series of lines that begin either with
 * the character sequence "< " (for deletions), or "> " (for additions).
 * For a change hunk, which has both a deletions section and an
 * additions section, the two sections are separated by a single
 * line containing the three-character sequence "---".
 * This function returns only characters that are actually part of
 * the lines to be deleted or added; characters from the special
 * sequences "< ", "> ", and "---\n" are not returned.
 * @param hdr  Data structure containing the header of the current
 * hunk.
 *
 * @param in  The stream from which hunks are being read.
 * @return  A character that is the next character in the current
 * line of the deletions section or additions section, unless the
 * end of the section has been reached, in which case the special
 * value EOS is returned.  If the hunk is ill-formed; for example,
 * if it contains a line that is not terminated by a newline character,
 * or if end-of-file is reached in the middle of the hunk, or a hunk
 * of change type is missing an additions section, then the special
 * value ERR (error) is returned.  The value ERR will also be returned
 * if this function is called after the current hunk has been completely
 * read, unless an intervening call to hunk_next() has been made to
 * advance to the next hunk in the input.  Once ERR has been returned,
 * then further calls to this function will continue to return ERR,
 * until a successful call to call to hunk_next() has successfully
 * advanced to the next hunk.
 */

/**
 * @brief  Get the next character from the data portion of the hunk.
 * Checks if it is a new line if it is a new line use the HUNK structure
 * to tell what type of hunk it is
 *
 * If it is a A then it should be "> " and if it is a D then it should be "< "
 * and if it is a C then it should be "< " to "---" to "> ".
 *
 * FIND A WAY TO USE THE RANGES TO DETERMINE IF THERE IS A VALID AMOUNT OF LINES
 * WE CAN DO THIS BY CHECKING HOW MANY '\n' THERE ARE IN THE BODY OF THE HUNK
 * BE AWARE THAT THE "---" IS NOT A LINE
 *
 * QUESTIONS TO ASK:
 * IS THE HUNK_GETC() UNDER A LOOP?
 */

int hunk_getc(HUNK *hp, FILE *in) {
    // TO BE IMPLEMENTED
    if (header_read == 0) {
        return ERR;
    }
    int read = fgetc(in);
    if (hp->type == HUNK_APPEND_TYPE) {
        if (read == '\n') {
            // If it is done with the first line of the body then it adds to the buffer with the correct format
            if (hunk_line == 1) {
                char_counter++;
                unsigned char second_byte = (char_counter >> 8);
                unsigned char first_byte = char_counter & 0xff;
                *pointer = (char)first_byte;
                pointer++;
                *pointer = (char)second_byte;
                *letter = read;
                letter++;
                pointer = letter;
                letter = letter + 2;
                hunk_line = 0;
                ungetc(read, in);
                return read;
            }
            hunk_line = 0;
            read = fgetc(in);
            if (read == '>') {
                read = fgetc(in);
                if (read == ' ') {
                    add_count++;
                    read = fgetc(in);
                    hunk_line = 1;
                    char_counter = 0;
                    *letter = read;
                    char_counter++;
                    if (add_count > hp->new_end - hp->new_start + 1) {
                        // fprintf("TOO MANY LINES");
                        unsigned char second_byte = (char_counter >> 8);
                        unsigned char first_byte = char_counter & 0xff;
                        *pointer = (char)first_byte;
                        pointer++;
                        *pointer = (char)second_byte;
                        *letter = read;
                        letter++;
                        pointer = letter;
                        letter = letter + 2;
                        hunk_line = 0;
                        return ERR;
                    }
                    letter++;
                    return read;
                } else {
                    // printf("when it is not > ");
                    return ERR;
                }
            }

            // If it reads a header then it will return EOS
            if (read >= '0' || read <= '9') {
                if ((hp->new_end - hp->new_start) + 1 != add_count) {
                    add_count = 0;
                    hunk_line = 0;
                    // printf("when line count doesn't match");
                    return ERR;
                }
                read = ungetc(read, in);
                valid = 1;
                hunk_line = 0;
                add_count = 0;
                // printf("1\n");
                return EOS;
            }

            // If it reads a EOF and it lines up with the range then it will return EOS
            if (read == EOF && (hp->new_end - hp->new_start) + 1 == add_count) {
                add_count = 0;
                hunk_line = 0;
                // printf("1\n");
                return EOS;
            } else {
                add_count = 0;
                hunk_line = 0;
                // printf("When line count doesnt add up\n");
                return ERR;
            }
        }

        // If the hunk line is valid then it will continue
        if (hunk_line == 1) {
            // If it does match up then it will return EOS with buffer updated
            if (read == EOF) {
                if (ungetc(read, in) != '\n') {
                    add_count = 0;
                    hunk_line = 0;
                    // printf("ran into eof while reading line\n");
                    return ERR;
                }
                char_counter++;
                unsigned char second_byte = (char_counter >> 8);
                unsigned char first_byte = char_counter & 0xff;
                *pointer = (char)first_byte;
                pointer++;
                *pointer = (char)second_byte;
                *letter = '\n';
                letter++;
                *letter = 0x0;
                letter++;
                *letter = 0x0;
                add_count = 0;
                hunk_line = 0;
                // printf("1\n");
                return EOS;
            }
            *letter = read;
            letter++;
            char_counter++;
            return read;
        } else {
            hunk_line = 0;
            add_count = 0;
            // printf("????\n");
            return ERR;
        }
    }
    if (hp->type == HUNK_DELETE_TYPE) {
        if (read == '\n') {
            // If it is done with the first line of the body then it adds to the buffer with the correct format
            if (hunk_line == 1) {
                char_counter++;
                unsigned char second_byte = (char_counter >> 8);
                unsigned char first_byte = char_counter & 0xff;
                *pointer2 = (char)first_byte;
                pointer2++;
                *pointer2 = (char)second_byte;
                *letter2 = read;
                letter2++;
                pointer2 = letter2;
                letter2 = letter2 + 2;
                hunk_line = 0;
                ungetc(read, in);
                return read;
            }
            hunk_line = 0;
            read = fgetc(in);
            if (read == '<') {
                read = fgetc(in);
                if (read == ' ') {
                    del_count++;
                    read = fgetc(in);
                    hunk_line = 1;
                    char_counter = 0;
                    *letter2 = read;
                    char_counter++;
                    if (del_count > hp->old_end - hp->old_start + 1) {
                        // printf("del_count: %d", del_count);
                        // printf("TOO MANY LINES");
                        unsigned char second_byte = (char_counter >> 8);
                        unsigned char first_byte = char_counter & 0xff;
                        *pointer2 = (char)first_byte;
                        pointer2++;
                        *pointer2 = (char)second_byte;
                        *letter2 = read;
                        letter2++;
                        pointer2 = letter2;
                        letter2 = letter2 + 2;
                        hunk_line = 0;
                        return ERR;
                    }
                    letter2++;
                    return read;
                } else {
                    // printf("when it is not < ");
                    return ERR;
                }
            }

            // If it reads a header then it will return EOS
            if (read >= '0' || read <= '9') {
                // printf("del_count: %d", del_count);
                if (hp->old_start != hp->old_end) {
                    if ((hp->old_end - hp->old_start) + 1 != del_count) {
                        del_count = 0;
                        hunk_line = 0;
                        // printf("when line count doesn't match");
                        return ERR;
                    }
                }
                read = ungetc(read, in);
                valid = 1;
                hunk_line = 0;
                del_count = 0;
                // printf("1\n");
                return EOS;
            }

            // If it reads a EOF and it lines up with the range then it will return EOS
            if (read == EOF && (hp->old_end - hp->old_start) + 1 == del_count) {
                del_count = 0;
                hunk_line = 0;
                // printf("1\n");
                return EOS;
            } else {
                del_count = 0;
                hunk_line = 0;
                // printf("When line count doesnt add up\n");
                return ERR;
            }
        }

        // If the hunk line is valid then it will continue
        if (hunk_line == 1) {
            // If it does match up then it will return EOS with buffer updated
            if (read == EOF) {
                if (ungetc(read, in) != '\n') {
                    add_count = 0;
                    hunk_line = 0;
                    // printf("ran into eof while reading line\n");
                    return ERR;
                }
                char_counter++;
                unsigned char second_byte = (char_counter >> 8);
                unsigned char first_byte = char_counter & 0xff;
                *pointer2 = (char)first_byte;
                pointer2++;
                *pointer2 = (char)second_byte;
                *letter2 = '\n';
                letter2++;
                *letter2 = 0x0;
                letter2++;
                *letter2 = 0x0;
                add_count = 0;
                hunk_line = 0;
                // printf("1\n");
                return EOS;
            }
            *letter2 = read;
            letter2++;
            char_counter++;
            return read;
        } else {
            hunk_line = 0;
            add_count = 0;
            // printf("????\n");
            return ERR;
        }
    }
    if (hp->type == HUNK_CHANGE_TYPE) {
        if (read == '\n') {
            if (hunk_line == 1 && part1 == 1 && part2 == 0) {
                char_counter++;
                unsigned char second_byte = (char_counter >> 8);
                unsigned char first_byte = char_counter & 0xff;
                *pointer2 = (char)first_byte;
                pointer2++;
                *pointer2 = (char)second_byte;
                *letter2 = read;
                letter2++;
                pointer2 = letter2;
                letter2 = letter2 + 2;
                hunk_line = 0;
                ungetc(read, in);
                return read;
            }
            if (hunk_line == 1 && part1 == 1 && part2 == 1) {
                char_counter++;
                unsigned char second_byte = (char_counter >> 8);
                unsigned char first_byte = char_counter & 0xff;
                *pointer = (char)first_byte;
                pointer++;
                *pointer = (char)second_byte;
                *letter = read;
                letter++;
                pointer = letter;
                letter = letter + 2;
                hunk_line = 0;
                ungetc(read, in);
                return read;
            }
            hunk_line = 0;
            read = fgetc(in);
            if (read == '<') {
                read = fgetc(in);
                if (read == ' ') {
                    del_count++;
                    read = fgetc(in);
                    hunk_line = 1;
                    char_counter = 0;
                    *letter2 = read;
                    char_counter++;
                    if (del_count > hp->old_end - hp->old_start + 1) {
                        // printf("TOO MANY LINES");
                        unsigned char second_byte = (char_counter >> 8);
                        unsigned char first_byte = char_counter & 0xff;
                        *pointer2 = (char)first_byte;
                        pointer2++;
                        *pointer2 = (char)second_byte;
                        *letter2 = read;
                        letter2++;
                        pointer2 = letter2;
                        letter2 = letter2 + 2;
                        hunk_line = 0;
                        return ERR;
                    }
                    part1 = 1;
                    letter2++;
                    return read;
                } else {
                    // printf("when it is not < ");
                    return ERR;
                }
            }
            if (read == '-') {
                read = fgetc(in);
                if (read == '-') {
                    read = fgetc(in);
                    if (read == '-') {
                        read = fgetc(in);
                        if (read == '\n') {
                            if (hp->old_start != hp->old_end) {
                                if (del_count != (hp->old_end - hp->old_start) + 1) {
                                    del_count = 0;
                                    hunk_line = 0;
                                    // printf("when line count doesn't match");
                                    return ERR;
                                }
                            }
                            read = fgetc(in);
                            part2 = 1;
                        } else {
                            // printf("when it is not --- new line");
                            return ERR;
                        }
                    } else {
                        // printf("missing -");
                        return ERR;
                    }
                } else {
                    // printf("missing --");
                    return ERR;
                }
            }
            if (read == '>' && part1 == 1 && part2 == 1) {
                read = fgetc(in);
                if (read == ' ') {
                    add_count++;
                    read = fgetc(in);
                    hunk_line = 1;
                    char_counter = 0;
                    // printf("letter %c", *letter);
                    *letter = read;
                    // printf("letter %c", *letter);
                    char_counter++;
                    if (add_count > hp->new_end - hp->new_start + 1) {
                        // printf("TOO MANY LINES");
                        unsigned char second_byte = (char_counter >> 8);
                        unsigned char first_byte = char_counter & 0xff;
                        *pointer = (char)first_byte;
                        pointer++;
                        *pointer = (char)second_byte;
                        *letter = read;
                        letter++;
                        pointer = letter;
                        letter = letter + 2;
                        hunk_line = 0;
                        return ERR;
                    }
                    letter++;
                    return read;
                } else {
                    // printf("when it is not > ");
                    return ERR;
                }
            }
            if (read >= '0' || read <= '9') {
                if ((hp->new_end - hp->new_start) + 1 != add_count) {
                    add_count = 0;
                    hunk_line = 0;
                    // printf("when line count doesn't match");
                    return ERR;
                }
                read = ungetc(read, in);
                valid = 1;
                hunk_line = 0;
                add_count = 0;
                // printf("readinthis 1 \n");
                return EOS;
            }

            if (read == EOF && (hp->new_end - hp->new_start) + 1 == add_count) {
                add_count = 0;
                hunk_line = 0;
                // printf("1231231\n");
                return EOS;
            } else {
                add_count = 0;
                hunk_line = 0;
                // printf("%c", read);
                // printf("When line count doesnt add up\n");
                return ERR;
            }
        }

        // If the hunk line is valid then it will continue
        if (hunk_line == 1) {
            // If it does match up then it will return EOS with buffer updated
            if (read == EOF && part1 == 1 && part2 == 0) {
                if (ungetc(read, in) != '\n') {
                    add_count = 0;
                    hunk_line = 0;
                    // printf("ran into eof while reading line WHILE READING DEL\n");
                    return ERR;
                }
                char_counter++;
                unsigned char second_byte = (char_counter >> 8);
                unsigned char first_byte = char_counter & 0xff;
                *pointer2 = (char)first_byte;
                pointer2++;
                *pointer2 = (char)second_byte;
                *letter2 = '\n';
                letter2++;
                *letter2 = 0x0;
                letter2++;
                *letter2 = 0x0;
                add_count = 0;
                hunk_line = 0;
                // printf("wasdwa\n");
                return EOS;
            }
            if (part1 == 1 && part2 == 0) {
                *letter2 = read;
                letter2++;
                char_counter++;
                return read;
            }

            if (read == EOF && part1 == 1 && part2 == 1) {
                if (ungetc(read, in) != '\n') {
                    add_count = 0;
                    hunk_line = 0;
                    // printf("ran into eof while reading line WHILE READING ADD\n");
                    return ERR;
                }
                char_counter++;
                unsigned char second_byte = (char_counter >> 8);
                unsigned char first_byte = char_counter & 0xff;
                *pointer = (char)first_byte;
                pointer++;
                *pointer = (char)second_byte;
                *letter = '\n';
                letter++;
                *letter = 0x0;
                letter++;
                *letter = 0x0;
                add_count = 0;
                hunk_line = 0;
                // printf("are we reading from this?\n");
                return EOS;
            }
            if (part2 == 1) {
                *letter = read;
                letter++;
                char_counter++;
                return read;
            }
        } else {
            hunk_line = 0;
            add_count = 0;
            return ERR;
        }
    }
    return ERR;
}

/**
 * @brief  Print a hunk to an output stream.
 * @details  This function prints a representation of a hunk to a
 * specified output stream.  The printed representation will always
 * have an initial line that specifies the type of the hunk and
 * the line numbers in the "old" and "new" versions of the file,
 * in the same format as it would appear in a traditional diff file.
 * The printed representation may also include portions of the
 * lines to be deleted and/or inserted by this hunk, to the extent
 * that they are available.  This information is defined to be
 * available if the hunk is the current hunk, which has been completely
 * read, and a call to hunk_next() has not yet been made to advance
 * to the next hunk.  In this case, the lines to be printed will
 * be those that have been stored in the hunk_deletions_buffer
 * and hunk_additions_buffer array.  If there is no current hunk,
 * or the current hunk has not yet been completely read, then no
 * deletions or additions information will be printed.
 * If the lines stored in the hunk_deletions_buffer or
 * hunk_additions_buffer array were truncated due to there having
 * been more data than would fit in the buffer, then this function
 * will print an elipsis "..." followed by a single newline character
 * after any such truncated lines, as an indication that truncation
 * has occurred.
 *
 * @param hp  Data structure giving the header information about the
 * hunk to be printed.
 * @param out  Output stream to which the hunk should be printed.
 */

void hunk_show(HUNK *hp, FILE *out) {
    // TO BE IMPLEMENTED
    int old_comma = 0;
    int new_comma = 0;
    int line_count_add = 0;
    int line_count_del = 0;
    if (hp->type == HUNK_APPEND_TYPE) {
        int i = 0;
        char *add_pointer = hunk_additions_buffer;
        fprintf(out, "%d", hp->old_start);
        fputc('a', out);
        fprintf(out, "%d", hp->new_start);
        if (hp->new_end != hp->new_start) {
            fputc(',', out);
            fprintf(out, "%d", hp->new_end);
        }
        fputc('\n', out);
        fputc('>', out);
        fputc(' ', out);
        i += 2;
        while (i < HUNK_MAX) {
            if (*add_pointer == '\n') {
                fputc('\n', out);
                add_pointer++;
                line_count_add++;
                i++;
                if (*add_pointer == 0) {
                    // printf("%d", *(add_pointer));
                    // printf("we are here\n");
                    add_pointer++;
                    if (*add_pointer == 0) {
                        // printf("%d", *(add_pointer));
                        // printf("we are here2\n");
                        add_pointer++;
                        i++;
                    } else {
                        add_pointer++;
                        i++;
                        fputc('>', out);
                        fputc(' ', out);
                    }
                } else {
                    add_pointer++;
                    i++;
                    fputc('>', out);
                    fputc(' ', out);
                }
            }
            fputc(*add_pointer, out);
            add_pointer++;
            i++;
        }
        char *max = hunk_additions_buffer + HUNK_MAX - 3;
        if (*max > 0) {
            fputc('.', out);
            fputc('.', out);
            fputc('.', out);
        }
        fputc('\n', out);
    }
    if (hp->type == HUNK_DELETE_TYPE) {
        int i = 0;
        fprintf(out, "%d", hp->old_start);
        char *del_pointer = hunk_deletions_buffer;
        if (hp->old_start != hp->old_end) {
            fputc(',', out);
            fprintf(out, "%d", hp->old_end);
        }

        fputc('d', out);
        fprintf(out, "%d", hp->new_start);
        fputc('\n', out);
        fputc('<', out);
        fputc(' ', out);
        i += 2;
        while (i < HUNK_MAX) {
            if (*del_pointer == '\n' && ((hp->old_end - hp->old_start + 1) != line_count_del)) {
                fputc('\n', out);
                del_pointer++;
                line_count_del++;
                i++;
                if (*del_pointer == 0) {
                    del_pointer++;
                    if (*del_pointer == 0) {
                        del_pointer++;
                        i++;
                    } else {
                        del_pointer++;
                        i++;
                        fputc('<', out);
                        fputc(' ', out);
                    }
                } else {
                    del_pointer++;
                    i++;
                    fputc('<', out);
                    fputc(' ', out);
                }
            }
            fputc(*del_pointer, out);
            del_pointer++;
            i++;
        }
        char *max = hunk_deletions_buffer + HUNK_MAX - 3;
        if (*max > 0) {
            fputc('.', out);
            fputc('.', out);
            fputc('.', out);
        }
        fputc('\n', out);
    }
    if (hp->type == HUNK_CHANGE_TYPE) {
        int i = 0;
        int j = 0;
        char *add_pointer = hunk_additions_buffer;
        char *del_pointer = hunk_deletions_buffer;
        fprintf(out, "%d", hp->old_start);
        if (hp->old_start != hp->old_end) {
            fputc(',', out);
            fprintf(out, "%d", hp->old_end);
        }
        fputc('c', out);
        fprintf(out, "%d", hp->new_start);
        if (hp->new_start != hp->new_end) {
            fputc(',', out);
            fprintf(out, "%d", hp->new_end);
        }
        fputc('\n', out);
        fputc('<', out);
        fputc(' ', out);
        i += 2;
        while (i < HUNK_MAX) {
            if (*del_pointer == '\n' && ((hp->old_end - hp->old_start + 1) != line_count_del)) {
                fputc('\n', out);
                del_pointer++;
                line_count_del++;
                i++;
                if (*del_pointer == 0) {
                    del_pointer++;
                    if (*del_pointer == 0) {
                        del_pointer++;
                        i++;
                    } else {
                        del_pointer++;
                        i++;
                        fputc('<', out);
                        fputc(' ', out);
                    }
                } else {
                    del_pointer++;
                    i++;
                    fputc('<', out);
                    fputc(' ', out);
                }
            }
            fputc(*del_pointer, out);
            del_pointer++;
            i++;
        }
        char *max = hunk_deletions_buffer + HUNK_MAX - 3;
        if (*max > 0) {
            fputc('.', out);
            fputc('.', out);
            fputc('.', out);
            fputc('\n', out);
        }
        fputc('-', out);
        fputc('-', out);
        fputc('-', out);
        fputc('\n', out);
        fputc('>', out);
        fputc(' ', out);
        j += 2;
        add_pointer += 2;
        while (j < HUNK_MAX) {
            if (*add_pointer == '\n' && ((hp->new_end - hp->new_start + 1) != line_count_add)) {
                fputc('\n', out);
                add_pointer++;
                line_count_add++;
                i++;
                if (*add_pointer == 0) {
                    add_pointer++;
                    if (*add_pointer == 0) {
                        add_pointer++;
                        i++;
                    } else {
                        add_pointer++;
                        i++;
                        fputc('>', out);
                        fputc(' ', out);
                    }
                } else {
                    add_pointer++;
                    i++;
                    fputc('>', out);
                    fputc(' ', out);
                }
            }
            fputc(*add_pointer, out);
            add_pointer++;
            j++;
        }
        char *max1 = hunk_additions_buffer + HUNK_MAX - 3;
        if (*max1 > 0) {
            fputc('.', out);
            fputc('.', out);
            fputc('.', out);
        }
        fputc('\n', out);
    } else {
    }
}

/**
 * @brief  Patch a file as specified by a diff.
 * @details  This function reads a diff file from an input stream
 * and uses the information in it to transform a source file, read on
 * another input stream into a target file, which is written to an
 * output stream.  The transformation is performed "on-the-fly"
 * as the input is read, without storing either it or the diff file
 * in memory, and errors are reported as soon as they are detected.
 * This mode of operation implies that in general when an error is
 * detected, some amount of output might already have been produced.
 * In case of a fatal error, processing may terminate prematurely,
 * having produced only a truncated version of the result.
 * In case the diff file is empty, then the output should be an
 * unchanged copy of the input.
 *
 * This function checks for the following kinds of errors: ill-formed
 * diff file, failure of lines being deleted from the input to match
 * the corresponding deletion lines in the diff file, failure of the
 * line numbers specified in each "hunk" of the diff to match the line
 * numbers in the old and new versions of the file, and input/output
 * errors while reading the input or writing the output.  When any
 * error is detected, a report of the error is printed to stderr.
 * The error message will consist of a single line of text that describes
 * what went wrong, possibly followed by a representation of the current
 * hunk from the diff file, if the error pertains to that hunk or its
 * application to the input file.  If the "quiet mode" program option
 * has been specified, then the printing of error messages will be
 * suppressed.  This function returns immediately after issuing an
 * error report.
 *
 * The meaning of the old and new line numbers in a diff file is slightly
 * confusing.  The starting line number in the "old" file is the number
 * of the first affected line in case of a deletion or change hunk,
 * but it is the number of the line *preceding* the addition in case of
 * an addition hunk.  The starting line number in the "new" file is
 * the number of the first affected line in case of an addition or change
 * hunk, but it is the number of the line *preceding* the deletion in
 * case of a deletion hunk.
 *
 * @param in  Input stream from which the file to be patched is read.
 * @param out Output stream to which the patched file is to be written.
 * @param diff  Input stream from which the diff file is to be read.
 * @return 0 in case processing completes without any errors, and -1
 * if there were errors.  If no error is reported, then it is guaranteed
 * that the output is complete and correct.  If an error is reported,
 * then the output may be incomplete or incorrect.
 */

int patch(FILE *in, FILE *out, FILE *diff) {
    // TO BE IMPLEMENTED
    // CHECK THE DIFF HEADER FROM DIFF FILE
    // DO THE TYPE OF HUNKS
    // if diff reads eof as the first then just copy the file1 to file2
    HUNK hp;
    int line_add = 1;
    int line_del = 0;
    int a = 0;
    int b = 0;
    int c = 0;
    int line_tracker = 1;
    int line_tracker2 = 1;
    while ((a = hunk_next(&hp, diff)) == 0) {
        c = hunk_getc(&hp, diff);
        if (hp.type == HUNK_APPEND_TYPE) {
            if (hp.old_start > hp.new_start) {
                if ((global_options & QUIET_OPTION) != QUIET_OPTION) {
                    fprintf(stderr, "ERROR: HUNK APPEND TYPE: OLD START IS GREATER THAN NEW START");
                    hunk_show(&hp, stderr);
                }
                // printf("ERROR: HUNK APPEND TYPE: OLD START IS GREATER THAN NEW START AND LESS THAN NEW END\n");
                return -1;
            }
            b = fgetc(in);
            if (line_tracker <= hp.old_start) {
                while (line_tracker <= hp.old_start && line_tracker != 1) {
                    if ((global_options & NO_PATCH_OPTION) != NO_PATCH_OPTION) {
                        fputc(b, out);
                    }
                    b = fgetc(in);
                    if (b == EOF) {
                        // printf("ERROR: HUNK APPEND TYPE: B IS EOF\n");
                        if ((global_options & QUIET_OPTION) != QUIET_OPTION) {
                            fprintf(stderr, "ERROR: HUNK APPEND TYPE: IS READING EOF\n");
                            hunk_show(&hp, stderr);
                        }
                        return -1;
                    }
                    ////printf("B: %c\n", b);
                    if (b == '\n') {
                        line_tracker++;
                        line_tracker2++;
                    }
                    ////printf("LINE TRACKER: %d\n\n\n", line_tracker);
                }
                if ((global_options & NO_PATCH_OPTION) != NO_PATCH_OPTION) {
                    fputc('\n', out);
                }
                line_tracker++;
            }
            if ((hp.new_start) == line_tracker2) {
                while ((c) != EOS) {
                    if (c == ERR) {
                        // printf("ERROR: HUNK APPEND TYPE: C IS ERR\n");
                        if ((global_options & QUIET_OPTION) != QUIET_OPTION) {
                            fprintf(stderr, "ERROR: HUNK APPEND TYPE: IS READING ERR\n");
                            hunk_show(&hp, stderr);
                        }
                        return -1;
                    }
                    if ((global_options & NO_PATCH_OPTION) != NO_PATCH_OPTION) {
                        fputc(c, out);
                    }
                    if (c == '\n') {
                        line_tracker2++;
                    }
                    c = hunk_getc(&hp, diff);
                }
            } else {
                // printf("ERROR: HUNK APPEND TYPE: NEW START IS NOT EQUAL TO LINE IN\n");
                // printf("HERE OR SOMETHING\n");
                if ((global_options & QUIET_OPTION) != QUIET_OPTION) {
                    fprintf(stderr, "ERROR: HUNK APPEND TYPE: NEW START IS NOT EQUAL\n");
                    hunk_show(&hp, stderr);
                }
                return -1;
            }
        }
        if (hp.type == HUNK_DELETE_TYPE) {
            ////printf("line tracker: %d\n\n\n\n", line_tracker);
            while (line_tracker <= hp.old_start && line_tracker != 1) {
                b = fgetc(in);
                // printf("b: %c\n", b);
                if ((global_options & NO_PATCH_OPTION) != NO_PATCH_OPTION) {
                    fputc(b, out);
                }
                if (b == '\n') {
                    line_tracker++;
                    line_tracker2++;
                }
            }
            // printf("LINE TRACKER: %d", line_tracker);
            while ((c) != EOS) {
                b = fgetc(in);
                if (c == ERR) {
                    // printf("ERROR: HUNK DELETE TYPE: C IS ERR\n");
                    if ((global_options & QUIET_OPTION) != QUIET_OPTION) {
                        fprintf(stderr, "ERROR: HUNK DELETE TYPE: IS READING ERR\n");
                        hunk_show(&hp, stderr);
                    }
                    return -1;
                }
                if (b != c) {
                    // printf("ERROR: HUNK DELETE TYPE: B IS NOT EQUAL TO C\n");
                    if ((global_options & QUIET_OPTION) != QUIET_OPTION) {
                        fprintf(stderr, "ERROR: HUNK DELETE TYPE: IS NOT EQUAL\n");
                        hunk_show(&hp, stderr);
                    }
                    return -1;
                }
                if (b == '\n') {
                    line_tracker++;
                }
                c = hunk_getc(&hp, diff);
            }
        }
        if (hp.type == HUNK_CHANGE_TYPE) {
            while (line_tracker <= hp.old_start && line_tracker != 1) {
                b = fgetc(in);
                if ((global_options & NO_PATCH_OPTION) != NO_PATCH_OPTION) {
                    fputc(b, out);
                }
                if (b == '\n') {
                    line_tracker++;
                    line_tracker2++;
                }
            }
            // printf("LINE TRACKER: %d", line_tracker);
            while (c != EOS && line_del != (hp.old_end - hp.old_start + 1)) {
                b = fgetc(in);
                ////printf("c: %c\n", c);
                ////printf("b: %c\n", b);
                if (c == ERR) {
                    // printf("ERROR: HUNK DELETE TYPE: C IS ERR\n");
                    if ((global_options & QUIET_OPTION) != QUIET_OPTION) {
                        fprintf(stderr, "ERROR: HUNK DELETE TYPE: IS READING ERR\n");
                        hunk_show(&hp, stderr);
                    }
                    return -1;
                }
                if (b != c) {
                    // printf("ERROR: HUNK DELETE TYPE: B IS NOT EQUAL TO C\n");
                    if ((global_options & QUIET_OPTION) != QUIET_OPTION) {
                        fprintf(stderr, "ERROR: HUNK DELETE TYPE: IS NOT EQUAL");
                        hunk_show(&hp, stderr);
                    }
                    return -1;
                }
                if (b == '\n') {
                    line_del++;
                    line_tracker++;
                }
                (c = hunk_getc(&hp, diff));
            }
            // printf("LINE DEL: %d", line_del);
            while ((c) != EOS) {
                if (c == ERR) {
                    // printf("ERROR: HUNK APPEND TYPE: C IS ERR\n");
                    if ((global_options & QUIET_OPTION) != QUIET_OPTION) {
                        fprintf(stderr, "ERROR: HUNK APPEND TYPE: IS READING ERR\n");
                        hunk_show(&hp, stderr);
                    }
                    return -1;
                }
                ////printf("c: %c\n", c);
                if ((global_options & NO_PATCH_OPTION) != NO_PATCH_OPTION) {
                    fputc(c, out);
                }
                if (c == '\n') {
                    line_tracker2++;
                }
                c = hunk_getc(&hp, diff);
            }
        }
        // printf("LINE count: %d", line_tracker);
        // printf("LINE count2: %d", line_tracker2);
        // printf("b: %c\n", b);
    }
    b = fgetc(in);
    if (b != EOF) {
        while (b != EOF) {
            if ((global_options & NO_PATCH_OPTION) != NO_PATCH_OPTION) {
                fputc(b, out);
            }
            b = fgetc(in);
            if (b == '\n') {
                line_tracker++;
                line_tracker2++;
            }
        }
    }
    return 0;
}
