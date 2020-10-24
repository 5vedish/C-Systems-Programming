#include "const.h"
#include "sequitur.h"

/*
 * Digram hash table.
 *
 * Maps pairs of symbol values to first symbol of digram.
 * Uses open addressing with linear probing.
 * See, e.g. https://en.wikipedia.org/wiki/Open_addressing
 */

/**
 * Clear the digram hash table.
 */
void init_digram_hash(void) {
    for ( int i = 0; i < MAX_DIGRAMS; i++){
        *(digram_table+i) = NULL;
    }
}

/**
 * Look up a digram in the hash table.
 *
 * @param v1  The symbol value of the first symbol of the digram.
 * @param v2  The symbol value of the second symbol of the digram.
 * @return  A pointer to a matching digram (i.e. one having the same two
 * symbol values) in the hash table, if there is one, otherwise NULL.
 */
SYMBOL *digram_get(int v1, int v2) {

    int index = DIGRAM_HASH(v1, v2);
    
    for (int i = index; (*(digram_table+i) != NULL) ; i++){
        
        if (i == MAX_DIGRAMS){ 
            i = 0;
        }

        SYMBOL *dp = *(digram_table+i); 

    if (*(digram_table+i) != TOMBSTONE){ 

        if (dp -> value == v1 && dp -> next -> value == v2){
            return *(digram_table+i);
        }
    }

    }

    return NULL;
}

/**
 * Delete a specified digram from the hash table.
 *
 * @param digram  The digram to be deleted.
 * @return 0 if the digram was found and deleted, -1 if the digram did
 * not exist in the table.
 *
 * Note that deletion in an open-addressed hash table requires that a
 * special "tombstone" value be left as a replacement for the value being
 * deleted.  Tombstones are treated as vacant for the purposes of insertion,
 * but as filled for the purpose of lookups.
 *
 * Note also that this function will only delete the specific digram that is
 * passed as the argument, not some other matching digram that happens
 * to be in the table.  The reason for this is that if we were to delete
 * some other digram, then somebody would have to be responsible for
 * recycling the symbols contained in it, and we do not have the information
 * at this point that would allow us to be able to decide whether it makes
 * sense to do it here.
 */
int digram_delete(SYMBOL *digram) {

    int v1 = digram -> value, v2 = digram -> next -> value;
    int index = DIGRAM_HASH(v1, v2);

    for (int i = index; (*(digram_table+i) != NULL) ; i++){
        
        if (i == MAX_DIGRAMS){
            i = 0;
        }

        SYMBOL *dp = *(digram_table+i);

        if (*(digram_table+i) != TOMBSTONE){

            if (dp -> value == v1 && dp -> next -> value == v2){

            *(digram_table+i) = TOMBSTONE;
            return 0;

        }

        }  
    }

    return -1;
}

/**
 * Attempt to insert a digram into the hash table.
 *
 * @param digram  The digram to be inserted.
 * @return  0 in case the digram did not previously exist in the table and
 * insertion was successful, 1 if a matching digram already existed in the
 * table and no change was made, and -1 in case of an error, such as the hash
 * table being full or the given digram not being well-formed.
 */
int digram_put(SYMBOL *digram) {

    int v1 = digram -> value, v2 = digram -> next -> value, 
    index = DIGRAM_HASH(v1,v2), isFull = 0;


    for (int i = 0; i < MAX_DIGRAMS; i++){
        if (*(digram_table+i) == NULL){
            isFull = 1;
            break;
        }
    }

    if (isFull == 0){
        return -1;
    }


    if (digram_get(v1, v2) == NULL){

        for (int i = index; i < MAX_DIGRAMS; i++){
            
            if (*(digram_table+i) == NULL || *(digram_table+i) == TOMBSTONE){ // if that position is null, put it there
                *(digram_table+i) = digram;
                return 0;
            }

            if (i == MAX_DIGRAMS){ // resets if it reaches end of array
                i = 0;
            }

        }

    } else {
        return 1;
    }
    return -1;
}
