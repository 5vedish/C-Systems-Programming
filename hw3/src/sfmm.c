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
sf_block *split(sf_block *tosplit, size_t size);
sf_block *ext_wil(sf_block *wil, size_t size);

//Personal Declarations
static int fibonacci[] = {1,2,3,5,8,13,21,34,35,0};
sf_block* epilogue; //bring this outside to keep track of it

void *sf_malloc(size_t size) {

    //nothing happens
    if (size == 0){
        return NULL;
    }

    static int heap_init = 0;
    int blocksize = 0;
    if (size%64 != 0){
        blocksize += (size + (64 - (size%64)))/64; //for memory alignment
    }


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

        epilogue = sf_mem_end(); //getting addr at end of heap
        char *endhep = (char *) epilogue;

        endhep -= 16; //fitting the epilogue (header) before end of heap
        epilogue = (sf_block *) endhep;

        epilogue -> header = THIS_BLOCK_ALLOCATED;

        stahep += 64; //to point to end of prologue
        sf_block* srt_wil = (sf_block *) stahep; //start of wilderness block and inserting it
        sf_free_list_heads[9].body.links.next = srt_wil;
        sf_free_list_heads[9].body.links.prev = srt_wil;

        srt_wil->body.links.prev = &sf_free_list_heads[9]; //maintain doubly linked
        srt_wil->body.links.next = &sf_free_list_heads[9];

        int wil_siz = (epilogue - srt_wil) * sizeof(sf_block); //getting size of wilderness block
        wil_siz = wil_siz | PREV_BLOCK_ALLOCATED;
        srt_wil->header = wil_siz;

        epilogue -> header =  (epilogue -> header) | PREV_BLOCK_ALLOCATED; //it's the wilderness

        heap_init = 1;
    }

    sf_block *all_blo = ret_free(blocksize); //retrieves the block to work on

    if (((all_blo -> header & BLOCK_SIZE_MASK)/64) == blocksize){
        // (all_blo -> body.links.prev) -> body.links.next = all_blo -> body.links.next; //removing from the doubly linked
        // (all_blo -> body.links.next) -> body.links.prev = all_blo -> body.links.prev; 
        // all_blo -> header = (all_blo -> header) | THIS_BLOCK_ALLOCATED; 
        //the stuff above is actually doing it twice, double check it when you get there
        return all_blo;
    } else {
        all_blo = split(all_blo, blocksize);
        return all_blo;
    }
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

        while (counter->body.links.next != counter){

            cot_siz = ((counter -> header) & BLOCK_SIZE_MASK)/64;
            
            if (cot_siz >= size){ //if a block greater than 34 can still hold this
                return counter;
            }
        counter = counter->body.links.next;
        }

        return sf_free_list_heads[9].body.links.next; //if it gets past prev loop you need to allocate from wilderness

}


//remember to pass in blocksize for size
sf_block *split(sf_block *tosplit, size_t size){

    int is_wil = 0;
    char *temp = (char *) tosplit + (tosplit -> header & BLOCK_SIZE_MASK); //temp char ptr
    sf_block* tempblock; //temp block ptr
    if ( (sf_block *) temp == epilogue){
        is_wil = 1;
    }

    
    sf_block *nxthed = tosplit -> body.links.next;
    sf_block *prvhed = tosplit -> body.links.prev;
    size_t blocksize = (tosplit->header & BLOCK_SIZE_MASK); //correct size of the block

    //if given an alloced block, unsets the next block's prevalloc if resize is smaller
    if ((size * 64) < blocksize && (tosplit -> header & THIS_BLOCK_ALLOCATED) == 1){
        temp = (char *) tosplit + (tosplit -> header & BLOCK_SIZE_MASK);
        tempblock = (sf_block *) temp;
        tempblock -> header = (tempblock -> header) & (~PREV_BLOCK_ALLOCATED);
    }

    int dif; //difference in actual bytes
    dif = blocksize - (size * 64); //getting the difference in bytes

    tosplit -> header = (tosplit -> header & PREV_BLOCK_ALLOCATED) | THIS_BLOCK_ALLOCATED; // preserve the prev alloc and set this to alloc
    
    tosplit -> header = tosplit -> header | (size*64); //setting new size

    nxthed -> body.links.prev = tosplit -> body.links.prev; //removing from doubly
    prvhed -> body.links.next = tosplit -> body.links.next;

    temp = (char *) tosplit + (tosplit -> header & BLOCK_SIZE_MASK); //arithmetic to get to new spot
    sf_block *newblock = (sf_block *) temp; //creating the pointer for it

    newblock -> header = dif | PREV_BLOCK_ALLOCATED;
    if (dif%64 != 0){
        dif = (dif + (64 - (dif%64)))/64; //if it isn't multiple, round it up
    } else {
        dif = dif/64; //just divide by 64 if multiple of 64
    }

    for (int i = 0; i < 9; i++){
        if ( dif <= fibonacci[i] || (is_wil == 0 && i == 8)){

            tempblock = sf_free_list_heads + i; //gets you to current index
            newblock -> body.links.prev = tempblock -> body.links.prev; //insert into doubly
            newblock -> body.links.next = tempblock;
            (tempblock -> body.links.prev) -> body.links.next = newblock;
            tempblock -> body.links.prev = newblock;
    
        }
    }

    //if it is the wilderness
    if (is_wil == 1){
            tempblock = sf_free_list_heads + 9; //gets you to current index
            newblock -> body.links.prev = tempblock -> body.links.prev; //insert into doubly
            newblock -> body.links.next = tempblock;
            (tempblock -> body.links.prev) -> body.links.next = newblock;
            tempblock -> body.links.prev = newblock;
    }

    return tosplit;
}

sf_block *ext_wil(sf_block *wil, size_t size){
    return wil;
}