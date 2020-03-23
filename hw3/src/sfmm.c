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
        blocksize += (size + (64 - (size%64)))/64; //for memory alignment


    if (heap_init == 0){
        //initialize heap (needed?)

        for (int i = 0; i < 10; i++){ //initializes the sentinel nodes
            sf_free_list_heads[i].body.links.next = &sf_free_list_heads[i];
            sf_free_list_heads[i].body.links.prev = &sf_free_list_heads[i];
        }

        sf_block *prologue = sf_mem_grow(); //grab new page
        char *stahep = (char *) prologue; //char ptr for arithmetic

        stahep += 48; //7 unused rows and overlaps with prologue prev_footer
        prologue = (sf_block *) stahep;

        prologue -> header = 64 | THIS_BLOCK_ALLOCATED | PREV_BLOCK_ALLOCATED; //setting header and footer

        epilogue = sf_mem_end(); //getting addr at end of heap
        char *endhep = (char *) epilogue;

        endhep -= 16; //fitting the epilogue (header) before end of heap
        epilogue = (sf_block *) endhep;

        epilogue -> header = THIS_BLOCK_ALLOCATED;

        stahep += 64; //to point to end of prologue
        sf_block* srt_wil = (sf_block *) stahep; //start of wilderness block and inserting it
        srt_wil -> prev_footer = prologue -> header;
        sf_free_list_heads[9].body.links.next = srt_wil;
        sf_free_list_heads[9].body.links.prev = srt_wil;

        srt_wil->body.links.prev = &sf_free_list_heads[9]; //maintain doubly linked
        srt_wil->body.links.next = &sf_free_list_heads[9];

        int wil_siz = (epilogue - srt_wil) * sizeof(sf_block); //getting size of wilderness block
        wil_siz = wil_siz | PREV_BLOCK_ALLOCATED;
        srt_wil->header = wil_siz;

        epilogue -> prev_footer = srt_wil -> header; //wilderness' footer is epilogue's prev footer

        heap_init = 1;
    }

    sf_block *all_blo = ret_free(blocksize); //retrieves the block to work on
    sf_block *tempblock;
    char *temp;

    if (((all_blo -> header & BLOCK_SIZE_MASK)/64) == blocksize){
        (all_blo -> body.links.prev) -> body.links.next = all_blo -> body.links.next; //removing from the doubly linked
        (all_blo -> body.links.next) -> body.links.prev = all_blo -> body.links.prev; 
        all_blo -> header = (all_blo -> header) | THIS_BLOCK_ALLOCATED;
        temp = (char *) all_blo + (all_blo -> header & BLOCK_SIZE_MASK); //getting the next block in memory
        tempblock = (sf_block *) temp;
        tempblock -> header = tempblock -> header | PREV_BLOCK_ALLOCATED; //setting allocation status of next block 
    } else {
        all_blo = split(all_blo, blocksize);
    }

    //if wilderness is zero, destroy it
    tempblock = &sf_free_list_heads[9];
    if ((tempblock -> body.links.next -> header & BLOCK_SIZE_MASK) == 0){
        tempblock -> body.links.next = tempblock;
        tempblock -> body.links.prev = tempblock;
    }

    return all_blo ->body.payload;
}

void sf_free(void *pp) {

    char *temp;
    sf_block *tempblock;

    //handle all the invalid pointer stuff first
    if (pp == NULL){
        abort();
    }

    pp -= 16;
    int error = 0;
    sf_block *to_free = (sf_block *) pp;

    if ((to_free -> header & THIS_BLOCK_ALLOCATED) != THIS_BLOCK_ALLOCATED){
        error = 1;
    }

    temp = (char *) sf_mem_start();
    temp += 64 + 48; //end of prologue
    sf_block *prologue = (sf_block *) temp;

    if (to_free < prologue || to_free > epilogue){
        error = 1;
    }

    if ((to_free -> header & BLOCK_SIZE_MASK)%64 != 0){
        error = 1;
    }

    if ((to_free -> header & PREV_BLOCK_ALLOCATED) == 0 && (to_free -> prev_footer & THIS_BLOCK_ALLOCATED) != 0){
        error = 1;
    }

    if (error ==  1){
        abort();
    }

    //coalesce first

    int prev_free = 1;
    int next_free = 1;
    int is_wil = 0;
    sf_block *prv_blc;
    sf_block *nxt_blc;
    sf_block *nxt_nxt;
    sf_block* to_put;
    size_t new_siz;

    temp = (char *) to_free - (to_free -> prev_footer & BLOCK_SIZE_MASK);
    prv_blc = (sf_block *) temp;

    if ((to_free -> header & PREV_BLOCK_ALLOCATED) == PREV_BLOCK_ALLOCATED){
        prev_free = 0;
    }

    temp = (char *) to_free + (to_free -> header & BLOCK_SIZE_MASK);
    nxt_blc = (sf_block *) temp;

    if ((nxt_blc -> header & THIS_BLOCK_ALLOCATED) == THIS_BLOCK_ALLOCATED){
        next_free = 0;
    }

    //get the second block after to free
    if (next_free == 1){
        temp = (char *) nxt_blc + (nxt_blc -> header & BLOCK_SIZE_MASK);
        nxt_nxt = (sf_block *) temp;
    }

    if (nxt_nxt == epilogue){
        is_wil = 1;
    }


    //1.if behind and front allocated
    //2.if behind allocated and front free
    //3.if behind free and front allocated
    //4.if behind and front free
    if (prev_free == 0 && next_free == 0){

        to_free -> header = to_free -> header & (~THIS_BLOCK_ALLOCATED); //unset the block alloc status
        nxt_blc -> header = nxt_blc -> header & (~PREV_BLOCK_ALLOCATED); //unset the prev alloc for next block
        nxt_blc -> prev_footer = to_free -> header; //setting the freed block's footer

        to_put = to_free;

    } else if (prev_free == 0 && next_free == 1){

        (nxt_blc -> body.links.prev) -> body.links.next = nxt_blc -> body.links.next; //remove from doubly
        (nxt_blc -> body.links.next) -> body.links.prev = nxt_blc -> body.links.prev;

        new_siz = (to_free -> header & BLOCK_SIZE_MASK) + (nxt_blc -> header & BLOCK_SIZE_MASK); //getting new size
        to_free -> header = new_siz | (to_free -> header & PREV_BLOCK_ALLOCATED); //changing the header

        nxt_nxt -> prev_footer = to_free -> header; //setting footer for freed block
        to_put = to_free;

    } else if (prev_free == 1 && next_free == 0){

        (prv_blc -> body.links.prev) -> body.links.next = prv_blc -> body.links.next; //remove from doubly
        (prv_blc -> body.links.next) -> body.links.prev = prv_blc -> body.links.prev;

        new_siz = (prv_blc -> header & BLOCK_SIZE_MASK) + (to_free -> header & BLOCK_SIZE_MASK); //getting new size
        prv_blc -> header = new_siz | (prv_blc -> header & PREV_BLOCK_ALLOCATED); //changing the header

        nxt_blc -> prev_footer = prv_blc -> header;
        to_put = prv_blc;

    } else {

        (nxt_blc -> body.links.prev) -> body.links.next = nxt_blc -> body.links.next; //remove both from doubly
        (nxt_blc -> body.links.next) -> body.links.prev = nxt_blc -> body.links.prev;
        (prv_blc -> body.links.prev) -> body.links.next = prv_blc -> body.links.next; 
        (prv_blc -> body.links.next) -> body.links.prev = prv_blc -> body.links.prev;

        new_siz = (prv_blc -> header & BLOCK_SIZE_MASK) + (to_free -> header & BLOCK_SIZE_MASK) + (nxt_blc -> header & BLOCK_SIZE_MASK);
        prv_blc -> header = new_siz | (prv_blc -> header & PREV_BLOCK_ALLOCATED); //changing the header

        nxt_nxt -> prev_footer = prv_blc -> header;
        to_put = prv_blc;

    }

    //free the block by putting it in list

    size_t fit = to_put -> header & BLOCK_SIZE_MASK;
    if (fit%64 != 0){
        fit = (fit + (64 - (fit%64)))/64; //if it isn't multiple, round it up
    } else {
        fit = fit/64;
    }

    for (int i = 0; i < 9; i++){
        if ( (fit <= fibonacci[i] && is_wil == 0) || (i == 8 && is_wil == 0)){

            tempblock = sf_free_list_heads + i; //gets you to current index
            to_put -> body.links.next = tempblock -> body.links.next; //insert into beginning of doubly
            to_put -> body.links.prev = tempblock;
            (tempblock -> body.links.next) -> body.links.prev = to_put;
            tempblock -> body.links.next = to_put;
            break;
        }
    }

    if (is_wil == 1){
        
        tempblock = sf_free_list_heads + 9; //gets you to current index
        to_put -> body.links.prev = tempblock -> body.links.prev; //insert into doubly
        to_put -> body.links.next = tempblock;
        (tempblock -> body.links.prev) -> body.links.next = to_put;
        tempblock -> body.links.prev = to_put;
        
    }

    return;
}

void *sf_realloc(void *pp, size_t rsize) {
    return NULL;
}

void *sf_memalign(size_t size, size_t align) {
    return NULL;
}

//Helper Methods

//Helper Methods For Malloc

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

        while (counter->body.links.next != &sf_free_list_heads[8]){

            cot_siz = ((counter->body.links.next -> header) & BLOCK_SIZE_MASK)/64;
            
            if (cot_siz >= size){ //if a block greater than 34 can still hold this
                return counter->body.links.next;
            }
        counter = counter->body.links.next;
        }

    //if wilderness is required and there is no wilderness
    sf_block *new_wil = sf_free_list_heads[9].body.links.next; //new wilderness
    block = &sf_free_list_heads[9]; //sentinel node for wilderness
    char *temp;
    if (block -> body.links.next == block){
        sf_mem_grow();
        new_wil = epilogue;
        new_wil -> header = new_wil -> header | 4096; //a new page in memory
        new_wil -> header = new_wil -> header & (~THIS_BLOCK_ALLOCATED); //unset alloc status
        temp = sf_mem_end();
        temp -= 16;
        epilogue = (sf_block *) temp;
        epilogue -> header = epilogue -> header | THIS_BLOCK_ALLOCATED;
        epilogue -> prev_footer = new_wil -> header;
        //add back into doubly
        new_wil -> body.links.prev = block;
        new_wil -> body.links.next = block;
        block -> body.links.next = new_wil;
        block -> body.links.prev = new_wil;
    } 
     if ((new_wil -> header & BLOCK_SIZE_MASK) < (size*64))  {
        ext_wil(new_wil, (size*64));
    }

        return new_wil; //if it gets past prev loop you need to allocate from wilderness

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

    tosplit -> header = (tosplit -> header & PREV_BLOCK_ALLOCATED ) | THIS_BLOCK_ALLOCATED; // preserve the prev alloc and set this to alloc
    
    tosplit -> header = tosplit -> header | (size*64); //setting new size

    nxthed -> body.links.prev = tosplit -> body.links.prev; //removing from doubly
    prvhed -> body.links.next = tosplit -> body.links.next;

    temp = (char *) tosplit + (tosplit -> header & BLOCK_SIZE_MASK); //arithmetic to get to new spot
    sf_block *newblock = (sf_block *) temp; //creating the pointer for it

    newblock -> header = dif | PREV_BLOCK_ALLOCATED;
    
    if (dif%64 != 0){
        dif = (dif + (64 - (dif%64)))/64; //if it isn't multiple, round it up
    } else {
        dif = dif/64;
    }
    
    for (int i = 0; i < 9; i++){
        if ( (dif <= fibonacci[i] && is_wil == 0) || (is_wil == 0 && i == 8)){

            tempblock = sf_free_list_heads + i; //gets you to current index
            newblock -> body.links.next = tempblock -> body.links.next; //insert into beginning of doubly
            newblock -> body.links.prev = tempblock;
            (tempblock -> body.links.next) -> body.links.prev = newblock;
            tempblock -> body.links.next = newblock;
            break;
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

    //setting the footer of the new block
    temp = (char *) newblock + (newblock -> header & BLOCK_SIZE_MASK);
    tempblock = (sf_block *) temp;
    tempblock -> prev_footer = newblock -> header;

    return tosplit;
}

sf_block *ext_wil(sf_block *wil, size_t size){

    size_t crt_siz = wil -> header & BLOCK_SIZE_MASK;
    int extend = 0;
    char *temp;

    while (crt_siz < size){
        crt_siz += 4096;
        extend++;
    }

    while (extend > 0){
        sf_mem_grow();
        extend--;
    }

    temp = sf_mem_end();
    temp -= 16;
    epilogue = (sf_block *) temp;
    epilogue -> header = epilogue -> header | THIS_BLOCK_ALLOCATED;

    int new_siz = (epilogue - wil) * (sizeof(sf_block)); //finding the difference
    wil -> header = new_siz | (wil -> header & PREV_BLOCK_ALLOCATED); //retain prev alloc status

    epilogue -> prev_footer = wil -> header; 

    return wil;
}

//