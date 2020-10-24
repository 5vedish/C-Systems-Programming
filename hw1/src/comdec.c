#include "const.h"
#include "sequitur.h"
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


#define ONE_BYTE_MASK 0x80 // 1000 0000
#define TWO_BYTES_MASK 0xE0 // 1110 0000
#define THREE_BYTES_MASK 0xF0 // 1111 0000
#define FOUR_BYTES_MASK 0xF8 // 1111 1000
#define VALID_BYTE_MASK 0xC0 // 1100 0000

#define ONE_BYTE 0x0 // 0
#define TWO_BYTES 0x6 // 110
#define THREE_BYTES 0xE // 1110
#define FOUR_BYTES 0x1E // 1 1110
#define VALID_BYTE 0x2 // 10

#define SOT 0x81
#define EOT 0x82
#define SOB 0x83
#define EOB 0x84
#define DELIMITER 0x85

#define LOWER_SEVEN 0x7F // 0111 1111
#define LOWER_SIX 0x3F // 0011 1111
#define LOWER_FIVE 0x1F // 0001 1111
#define LOWER_FOUR 0xF // 0000 1111
#define LOWER_THREE 0x7 // 0000 0111

//masks below are for compression

#define INTERVAL1 0x0000
#define INTERVAL12 0x007F
#define INTERVAL2 0x0080
#define INTERVAL23 0x07FF
#define INTERVAL3 0x0800
#define INTERVAL34 0xFFFF
#define INTERVAL4 0x10000
#define INTERVAL4L 0x10FFFF

#define KEEP1 0x7F
#define KEEP2 0xDF
#define KEEP3 0xEF
#define KEEP4 0xF7
#define KEEPLOWER6 0xBF

extern SYMBOL *add_symbol(SYMBOL *s, SYMBOL *rule);
int expand();
extern SYMBOL *recycled_symbols;

/*
 * You may modify this file and/or move the functions contained here
 * to other source files (except for main.c) as you wish.
 *
 * IMPORTANT: You MAY NOT use any array brackets (i.e. [ and ]) and
 * you MAY NOT declare any arrays or allocate any storage with malloc().
 * The purpose of this restriction is to force you to use pointers.
 * Variables to hold the pathname of the current file or directory
 * as well as other data have been pre-declared for you in const.h.
 * You must use those variables, rather than declaring your own.
 * IF YOU VIOLATE THIS RESTRICTION, YOU WILL GET A ZERO!
 *
 * IMPORTANT: You MAY NOT use floating point arithmetic or declare
 * any "float" or "double" variables.  IF YOU VIOLATE THIS RESTRICTION,
 * YOU WILL GET A ZERO!
 */

/**
 * Main compression function.
 * Reads a sequence of bytes from a specified input stream, segments the
 * input data into blocks of a specified maximum number of bytes,
 * uses the Sequitur algorithm to compress each block of input to a list
 * of rules, and outputs the resulting compressed data transmission to a
 * specified output stream in the format detailed in the header files and
 * assignment handout.  The output stream is flushed once the transmission
 * is complete.
 *
 * The maximum number of bytes of uncompressed data represented by each
 * block of the compressed transmission is limited to the specified value
 * "bsize".  Each compressed block except for the last one represents exactly
 * "bsize" bytes of uncompressed data and the last compressed block represents
 * at most "bsize" bytes.
 *
 * @param in  The stream from which input is to be read.
 * @param out  The stream to which the block is to be written.
 * @param bsize  The maximum number of bytes read per block.
 * @return  The number of bytes written, in case of success,
 * otherwise EOF.
 */
int compress(FILE *in, FILE *out, int bsize) {

    fputc(0x81, stdout);

    init_symbols();
    init_rules();
    init_digram_hash();
    SYMBOL *sp = NULL;
    add_rule(new_rule(256));

    int c, counter = bsize;

    while ((c = fgetc(in)) != EOF){

        if (counter == 0){ //do the resetting stuff here
            fputc(0x84, stdout);
            init_symbols();
            init_rules();
            init_digram_hash();
            recycled_symbols = NULL;
            counter = bsize;
        }

        if (counter == bsize){
            fputc(0x83, stdout);
        }
        
        sp = new_symbol(c,NULL);

        insert_after(main_rule -> prev, sp);
        
        int check = check_digram(main_rule -> prev -> prev);
        printf("%d", check);

        counter--;
    }

    if (counter > 0){
        fputc(0x84, stdout);
    }

    fputc(0x82, stdout);

    return 0; //END OF PROGRESS

    int value, loop, temp = 0, temp2, choice;

    if (value > INTERVAL1 && value < INTERVAL12){
        
        choice = 1;

    } else if (value > INTERVAL2 && value < INTERVAL23){
        
        loop = 1;
        choice = 2;

    } else if (value > INTERVAL3 && value < INTERVAL34){

        loop = 2;
        choice = 3;
  
    } else if (value > INTERVAL4 && value < INTERVAL4L){

        loop = 3;
        choice = 4;
 
    }

    for (int i = 0; i < loop; i++){
        temp2 = value & LOWER_SIX;
        temp2 = temp2 & KEEPLOWER6;
        temp2 = temp2 << (i*8);
        
        temp = temp | temp2;
        value = value >> 6;

    }

    if (choice == 1){
        value = value & KEEP1;

    } else if (choice == 2){
        value = value & KEEP2;
        value = value << 8;

    } else if (choice == 3){
        value = value & KEEP3;
        value = value << 16;
        
    } else if (choice == 4){
        value = value & KEEP4;
        value = value << 24;
        
    }

    value = value | temp;

    return 0;

    init_digram_hash();

    SYMBOL *test = new_symbol(1,NULL);
    test -> next = new_symbol(2, NULL);

    printf("%d",test -> value);
    printf("%d",test -> next -> value);

    digram_put(test);
    digram_delete(test);
    digram_put(test);

    SYMBOL *test2 = digram_get(test-> value, test -> next -> value);

    printf("\n%d", test2 -> value);

    return 0;
    return EOF;
}

/**
 * Main decompression function.
 * Reads a compressed data transmission from an input stream, expands it,
 * and and writes the resulting decompressed data to an output stream.
 * The output stream is flushed once writing is complete.
 *
 * @param in  The stream from which the compressed block is to be read.
 * @param out  The stream to which the uncompressed data is to be written.
 * @return  The number of bytes written, in case of success, otherwise EOF.
 */
int decompress(FILE *in, FILE *out) {

    init_symbols();
    init_rules();
    int num_bytes = 0;

    SYMBOL *rp = NULL;

    int c, i = 0, in_block = 1, rule_reset = 1; // the byte, the index, whether it's in a block or not
    while ((c = fgetc(in)) != EOT){

        if (c != SOT && i == 0){
            return EOF; //throw error if not start of transmission
        }

        if (c == EOF){
            return EOF; // if end of transmission is missing
        }
    
        if (c == EOB){ // if end of block is hit, then it is out of block

            num_bytes += expand(main_rule);
            init_rules();
            rp = NULL;
            in_block = 1;
            rule_reset = 1;
        }

        if (c == DELIMITER){ 
            rule_reset = 1;
        }


    if (in_block == 0 && c != 0x85){ //ensures that it doesn't accidecentally take in delimiter

        int byte_loop = 0, counter, value = 0;

        if (((c & FOUR_BYTES_MASK) >> 3) == FOUR_BYTES){ // to find out how many bytes to take in
            byte_loop = 3;
            value = c & LOWER_THREE;

        } else if (((c & THREE_BYTES_MASK) >> 4) == THREE_BYTES){
            byte_loop = 2;
            value = c & LOWER_FOUR;
            
        } else if (((c & TWO_BYTES_MASK) >> 5) == TWO_BYTES){
            byte_loop = 1;
            value = c & LOWER_FIVE;
            
        } else if (((c & ONE_BYTE_MASK) >> 7) == ONE_BYTE){
            value = c & LOWER_SEVEN;
        } else {
            return EOF;
        }

        int valid;

        for (counter = 0; counter < byte_loop; counter++){ // to check if it's a valid continutation byte
            c = fgetc(in);
            
            if (((valid = (c & VALID_BYTE_MASK)) >> 6) != VALID_BYTE){
                return EOF;
            }

            c = c & LOWER_SIX;
            value = value << 6;
            value = value | c;
        }

        if (rule_reset == 1){
            add_rule((*(rule_map+value) = new_rule(value)));
            
            if (rp == NULL){
                rp = main_rule;
            } else {
                rp = rp -> nextr;
            }

            rule_reset = 0;
        } else {
            add_symbol(new_symbol(value, NULL), rp);
        }

    }

    if (in_block == 1){ // if it's out of block
        if (c == SOB){ // it's not in block
            in_block = 0;

        }
    } 

        i++;
    }

    return EOF;
}

/**
 * @brief Validates command line arguments passed to the program.
 * @details This function will validate all the arguments passed to the
 * program, returning 0 if validation succeeds and -1 if validation fails.
 * Upon successful return, the selected program options will be set in the
 * global variable "global_options", where they will be accessible
 * elsewhere in the program.
 *
 * @param argc The number of arguments passed to the program from the CLI.
 * @param argv The argument strings passed to the program from the CLI.
 * @return 0 if validation succeeds and -1 if validation fails.
 * Refer to the homework document for the effects of this function on
 * global variables.
 * @modifies global variable "global_options" to contain a bitmap representing
 * the selected options.
 */
int validargs(int argc, char **argv){

    global_options = 0;
    int index;
    int d_mask = 0x00000004;
    int c_mask = 0x00000002;
    int h_mask = 0x00000001;
    int def_glo_mask = 0x04000000;
    
    if (argc < 2 || argc == 0){ //if there aren't enough args
        return -1;
    }

    if ((*(*(argv+1)) == '-' && *((*(argv+1))+1)  == 'h')){ //checking for the -h flag
        if (*((*(argv+1))+2) != '\0'){
            return -1;
        }
        
        global_options = global_options | h_mask;
        return 0;
    }

    if (argc > 4){ //if there are too many arguments
        return -1;
    }

    for (index = 0; index < argc - 1; index++){
        ++argv;

        if (*(*argv+2) != '\0' && index != 2){ //arguments can't be greater than 3 chars except for blocksize
            return -1;
        }

        if (**argv == '-' && *(*argv+1) == 'd'){ //checking for -d flag and returns invalid if there's an argument aftere it
            if (*(argv+1) != NULL || index != 0){  
                global_options = 0;          
                return -1;
            }
        
        global_options = global_options | d_mask; //setting the third rightmost bit
            return 0;
        }

        if (**argv == '-' && *(*argv+1) == 'c'){ //checking for -c flag and its proper index
            if (index != 0){
                global_options = 0;
                return -1;
            }

        global_options = global_options | c_mask; //setting the second rightmost bit 


            if (*(argv+1) == NULL){ //if there's no optional command next
                return 0;
            }
        }

        if (**argv == '-' && *(*argv+1) == 'b'){ //checking for -b flag
            if (index != 1){ //if it's not in right position as third arg
                global_options = 0;
                return -1;
            }

            if (*(argv+1) == NULL){ //if the next one is null, set def global options
                global_options = global_options | def_glo_mask;
                return 0;
            }

        }

       if (*(*(argv-1)) == '-' &&  *((*(argv-1)+1)) == 'b'){ //if the previous one is -b then AND block to global options

           int block = 0;

           for (int i = 0; *((*argv)+i); i++){

               int digit = (*((*argv)+i)-48);

               if (digit < 0 || digit > 9){ //calculating the decimal val by going through char of number digits
                   global_options = 0;
                   return -1;
               }

               block = block * 10 + digit;

           }

           if(block > 1024 || block < 1){ //if it's not between 0-1024
               global_options = 0;
               return -1;
           }

           block = block << 16;
           global_options = global_options | block;

           return 0;
       }

    }

    global_options = 0;
    return -1;
}

int expand(SYMBOL *symbol){

    SYMBOL *rp = symbol;
    int num_bytes = 0;

    rp = rp -> next;

    if (rp == main_rule){ 
        return num_bytes;
    }

    while (rp != symbol){

        if (rp -> value < FIRST_NONTERMINAL){
            fputc(rp -> value, stdout);
            num_bytes++;
        } else if (rp -> value >= FIRST_NONTERMINAL){
            num_bytes += expand(*(rule_map+(rp -> value)));
        }

        rp = rp -> next;
    }

    return num_bytes;
}