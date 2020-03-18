/**
 * All functions you make for the assignment must be implemented in this file.
 * Do not submit your assignment with a main function in this file.
 * If you submit with a main function in this file, you will get a zero.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "debug.h"
#include "sfmm.h"

//Headers For Helpers
sf_block* ret_free(size_t size);

//Personal Array
static int fibonacci[] = {1,2,3,5,8,13,21,34,35,0};

void *sf_malloc(size_t size) {

    //nothing happens
    if (size == 0){
        return NULL;
    }

    //cannot allocate less than 64 (throw error?)
    if (size < 64){

    }

    static int heap_init = 0;
    int blocksize = 0;
    blocksize += size + (64 - (size%64)); //for memory alignment

    if (heap_init == 0){
        //initialize heap (needed?)
        sf_mem_init();

        for (int i = 0; i < 10; i++){ //initializes the sentinel nodes
            sf_free_list_heads[i].body.links.next = &sf_free_list_heads[i];
            sf_free_list_heads[i].body.links.prev = &sf_free_list_heads[i];
        }

        sf_block *prologue = sf_mem_grow(); //grab new page
        char *stahep = (char *) prologue; //char ptr for arithmetic

        stahep += 56; //7 unused rows
        prologue = (sf_block *) stahep;

        prologue -> header = 64 | THIS_BLOCK_ALLOCATED | PREV_BLOCK_ALLOCATED; //setting header and footer
        prologue -> prev_footer = prologue -> header;

        sf_header *epilogue = sf_mem_end(); //getting addr at end of heap
        char *endhep = (char *) epilogue;

        endhep -= 8; //fitting the epilogue (header) before end of heap
        epilogue = (sf_header *) endhep;

        *epilogue = 0 | THIS_BLOCK_ALLOCATED;

        heap_init = 1;
    }


    sf_block testblock;
    testblock.header = 15;
    sf_free_list_heads[4].body.links.next = &testblock;
    sf_block *test2 = ret_free(8);

    printf("%lu", test2 -> header);

    
    









    return NULL;
}

void sf_free(void *pp) {
    return;
}

void *sf_realloc(void *pp, size_t rsize) {
    return NULL;
}

void *sf_memalign(size_t size, size_t align) {
    return NULL;
}

//Helper Methods

//Helper Method To Traverse Array

sf_block *ret_free(size_t size){

    sf_block *block = sf_free_list_heads;
    int index; //index to return if there is a block

    for (index = 0; index < 8; index++){


        if (block->body.links.next != block){

            if (size <= fibonacci[index]){
                break; //return that block
            }
        }

        block++;
    }

    return (sf_free_list_heads[index].body.links.next);

    if (size >= fibonacci[9]){ //if greater than 34

    }




    return (sf_free_list_heads[index].body.links.next);
}