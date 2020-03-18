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

    static int heap_init = 0;
    int blocksize = 0;
    blocksize += (size + (64 - (size%64)))/64; //for memory alignment

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

        stahep += 64; //to point to end of prologue
        sf_block* srt_wil = (sf_block *) stahep; //start of wilderness block
        sf_free_list_heads[9].body.links.next = srt_wil;

        srt_wil->body.links.prev = &sf_free_list_heads[9]; //maintain doubly linked
        srt_wil->body.links.next = &sf_free_list_heads[9];

        int wil_siz = (epilogue - (sf_header *) srt_wil) * sizeof(sf_header); //getting size of wilderness block
        wil_siz = wil_siz | PREV_BLOCK_ALLOCATED;
        srt_wil->header = wil_siz;

        heap_init = 1;
    }

    // sf_block *all_blo = ret_free(blocksize);





    


    // sf_block testblock;
    // testblock.header = 3000;
    // sf_free_list_heads[8].body.links.next = &testblock;
    // sf_block *test2 = ret_free(blocksize);
    // printf("%lu", test2 -> header);



    
    









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
    int cot_siz; //to retrieve size from header

    for (index = 0; index < 8; index++){

        if (block->body.links.next != block){

            if (size <= fibonacci[index]){
                return (sf_free_list_heads[index].body.links.next); //return that block
            }

        }

        block++;
    }

   
    sf_block *counter = sf_free_list_heads+8; //gets me to the ninth index of free list array

    if (size >= fibonacci[8]){ //if greater than 34

        while (counter->body.links.next != counter){

            cot_siz = ((counter -> header) & BLOCK_SIZE_MASK)/64;
            
            if (cot_siz >= size){ //if a block greater than 34 can still hold this
                return counter;
            }
        counter = counter->body.links.next;
        }

    } 
        return sf_free_list_heads[9].body.links.next; //if it gets past prev loop you need to allocate from wilderness

}

