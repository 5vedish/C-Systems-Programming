#include <stdio.h>
#include <stdlib.h>

#include "const.h"
#include "debug.h"

#ifdef _STRING_H
#error "Do not #include <string.h>. You will get a ZERO."
#endif

#ifdef _STRINGS_H
#error "Do not #include <strings.h>. You will get a ZERO."
#endif

#ifdef _CTYPE_H
#error "Do not #include <ctype.h>. You will get a ZERO."
#endif

#define H_MASK 0x1
#define C_MASK 0x2
#define D_MASK 0x4

int main(int argc, char **argv)
{
    int ret;
    if(validargs(argc, argv))
        USAGE(*argv, EXIT_FAILURE);
    debug("Options: 0x%x", global_options);
    if(global_options & 1)
        USAGE(*argv, EXIT_SUCCESS);

 if ((global_options & D_MASK) == D_MASK){
     decompress(stdin, stdout);
 }   else if ((global_options & C_MASK) == C_MASK){
     compress(stdin, stdout, 1024);
 }   



    return EXIT_SUCCESS;
}

/*
 * Just a reminder: All non-main functions should
 * be in another file not named main.c
 */
