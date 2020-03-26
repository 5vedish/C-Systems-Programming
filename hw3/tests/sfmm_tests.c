#include <criterion/criterion.h>
#include <errno.h>
#include <signal.h>
#include "debug.h"
#include "sfmm.h"

void assert_free_block_count(size_t size, int count);
void assert_free_list_block_count(size_t size, int count);

/*
 * Assert the total number of free blocks of a specified size.
 * If size == 0, then assert the total number of all free blocks.
 */
void assert_free_block_count(size_t size, int count) {
    int cnt = 0;
    for(int i = 0; i < NUM_FREE_LISTS; i++) {
	sf_block *bp = sf_free_list_heads[i].body.links.next;
	while(bp != &sf_free_list_heads[i]) {
	    if(size == 0 || size == (bp->header & BLOCK_SIZE_MASK))
		cnt++;
	    bp = bp->body.links.next;
	}
    }
    if(size == 0) {
	cr_assert_eq(cnt, count, "Wrong number of free blocks (exp=%d, found=%d)",
		     count, cnt);
    } else {
	cr_assert_eq(cnt, count, "Wrong number of free blocks of size %ld (exp=%d, found=%d)",
		     size, count, cnt);
    }
}

/*
 * Assert that the free list with a specified index has the specified number of
 * blocks in it.
 */
void assert_free_list_size(int index, int size) {
    int cnt = 0;
    sf_block *bp = sf_free_list_heads[index].body.links.next;
    while(bp != &sf_free_list_heads[index]) {
	cnt++;
	bp = bp->body.links.next;
    }
    cr_assert_eq(cnt, size, "Free list %d has wrong number of free blocks (exp=%d, found=%d)",
		 index, size, cnt);
}

Test(sf_memsuite_student, malloc_an_int, .init = sf_mem_init, .fini = sf_mem_fini) {
	sf_errno = 0;
	int *x = sf_malloc(sizeof(int));

	cr_assert_not_null(x, "x is NULL!");

	*x = 4;

	cr_assert(*x == 4, "sf_malloc failed to give proper space for an int!");

	assert_free_block_count(0, 1);
	assert_free_block_count(3904, 1);
	assert_free_list_size(NUM_FREE_LISTS-1, 1);

	cr_assert(sf_errno == 0, "sf_errno is not zero!");
	cr_assert(sf_mem_start() + PAGE_SZ == sf_mem_end(), "Allocated more than necessary!");
}

Test(sf_memsuite_student, malloc_three_pages, .init = sf_mem_init, .fini = sf_mem_fini) {
	sf_errno = 0;
	// We want to allocate up to exactly three pages.
	void *x = sf_malloc(3 * PAGE_SZ - ((1 << 6) - sizeof(sf_header)) - 64 - 2*sizeof(sf_header));

	cr_assert_not_null(x, "x is NULL!");
	assert_free_block_count(0, 0);
	cr_assert(sf_errno == 0, "sf_errno is not 0!");
}

Test(sf_memsuite_student, malloc_too_large, .init = sf_mem_init, .fini = sf_mem_fini) {
	sf_errno = 0;
	void *x = sf_malloc(PAGE_SZ << 16);

	cr_assert_null(x, "x is not NULL!");
	assert_free_block_count(0, 1);
	assert_free_block_count(65408, 1);
	cr_assert(sf_errno == ENOMEM, "sf_errno is not ENOMEM!");
}

Test(sf_memsuite_student, free_quick, .init = sf_mem_init, .fini = sf_mem_fini) {
	sf_errno = 0;
	/* void *x = */ sf_malloc(8);
	void *y = sf_malloc(32);
	/* void *z = */ sf_malloc(1);

	sf_free(y);

	assert_free_block_count(0, 2);
	assert_free_block_count(64, 1);
	assert_free_block_count(3776, 1);
	cr_assert(sf_errno == 0, "sf_errno is not zero!");
}

Test(sf_memsuite_student, free_no_coalesce, .init = sf_mem_init, .fini = sf_mem_fini) {
	sf_errno = 0;
	/* void *x = */ sf_malloc(8);
	void *y = sf_malloc(200);
	/* void *z = */ sf_malloc(1);

	sf_free(y);

	assert_free_block_count(0, 2);
	assert_free_block_count(256, 1);
	assert_free_block_count(3584, 1);
	cr_assert(sf_errno == 0, "sf_errno is not zero!");
}

Test(sf_memsuite_student, free_coalesce, .init = sf_mem_init, .fini = sf_mem_fini) {
	sf_errno = 0;
	/* void *w = */ sf_malloc(8);
	void *x = sf_malloc(200);
	void *y = sf_malloc(300);
	/* void *z = */ sf_malloc(4);

	sf_free(y);
	sf_free(x);

	assert_free_block_count(0, 2);
	assert_free_block_count(576, 1);
	assert_free_block_count(3264, 1);
	cr_assert(sf_errno == 0, "sf_errno is not zero!");
}

Test(sf_memsuite_student, freelist, .init = sf_mem_init, .fini = sf_mem_fini) {
	void *u = sf_malloc(200);
	/* void *v = */ sf_malloc(300);
	void *w = sf_malloc(200);
	/* void *x = */ sf_malloc(500);
	void *y = sf_malloc(200);
	/* void *z = */ sf_malloc(700);

	sf_free(u);
	sf_free(w);
	sf_free(y);

	assert_free_block_count(0, 4);
	assert_free_block_count(256, 3);
	assert_free_block_count(1600, 1);
	assert_free_list_size(3, 3);
	assert_free_list_size(NUM_FREE_LISTS-1, 1);

	// First block in list should be the most recently freed block.
	int i = 3;
	sf_block *bp = sf_free_list_heads[i].body.links.next;
	cr_assert_eq(bp, (char *)y - 2*sizeof(sf_header),
		     "Wrong first block in free list %d: (found=%p, exp=%p)",
                     i, bp, (char *)y - 2*sizeof(sf_header));
}

Test(sf_memsuite_student, realloc_larger_block, .init = sf_mem_init, .fini = sf_mem_fini) {
	void *x = sf_malloc(sizeof(int));
	/* void *y = */ sf_malloc(10);
	x = sf_realloc(x, sizeof(int) * 20);

	cr_assert_not_null(x, "x is NULL!");
	sf_block *bp = (sf_block *)((char *)x - 2*sizeof(sf_header));
	cr_assert(bp->header & THIS_BLOCK_ALLOCATED, "Allocated bit is not set!");
	cr_assert((bp->header & BLOCK_SIZE_MASK) == 128, "Realloc'ed block size not what was expected!");

	assert_free_block_count(0, 2);
	assert_free_block_count(64, 1);
	assert_free_block_count(3712, 1);
}

Test(sf_memsuite_student, realloc_smaller_block_splinter, .init = sf_mem_init, .fini = sf_mem_fini) {
	void *x = sf_malloc(sizeof(int) * 20);
	void *y = sf_realloc(x, sizeof(int) * 16);

	cr_assert_not_null(y, "y is NULL!");
	cr_assert(x == y, "Payload addresses are different!");

	sf_block *bp = (sf_block *)((char*)y - 2*sizeof(sf_header));
	cr_assert(bp->header & THIS_BLOCK_ALLOCATED, "Allocated bit is not set!");
	cr_assert((bp->header & BLOCK_SIZE_MASK) == 128, "Block size not what was expected!");

	// There should be only one free block of size 3840.
	assert_free_block_count(0, 1);
	assert_free_block_count(3840, 1);
}

Test(sf_memsuite_student, realloc_smaller_block_free_block, .init = sf_mem_init, .fini = sf_mem_fini) {
	void *x = sf_malloc(sizeof(double) * 8);
	void *y = sf_realloc(x, sizeof(int));

	cr_assert_not_null(y, "y is NULL!");

	sf_block *bp = (sf_block *)((char*)y - 2*sizeof(sf_header));
	cr_assert(bp->header & THIS_BLOCK_ALLOCATED, "Allocated bit is not set!");
	cr_assert((bp->header & BLOCK_SIZE_MASK) == 64, "Realloc'ed block size not what was expected!");

	// After realloc'ing x, we can return a block of size 64 to the freelist.
	// This block will go into the main freelist and be coalesced.
	assert_free_block_count(0, 1);
	assert_free_block_count(3904, 1);
}

//############################################
//STUDENT UNIT TESTS SHOULD BE WRITTEN BELOW
//DO NOT DELETE THESE COMMENTS
//############################################

//Consecutive allocations and consecutvie calls to free respectively, all free blocks absorbed into wilderness when the last block is freed
Test(sf_memsuite_student, free_into_wilderness_cases, .init = sf_mem_init, .fini = sf_mem_fini) {

	void *x = sf_malloc(sizeof(int));
    void *y = sf_malloc(sizeof(int));
    void *z = sf_malloc(sizeof(int));
    
    sf_free(x);
    sf_free(y);
    sf_free(z);

	assert_free_block_count(64, 0); //all ints were freed properly 
	assert_free_list_size(NUM_FREE_LISTS-1, 1); //check that it's the wilderness
	assert_free_block_count(PAGE_SZ - 48 - 64 -16, 1); //they were coalesced into the wilderness correctly

}

//Destroy and remake the wilderness
Test(sf_memsuite_student, rebirth, .init = sf_mem_init, .fini = sf_mem_fini) {

	void *x = sf_malloc(PAGE_SZ - 48 - 64 - 16 - 8); //allocate just enough to destroy the heap
	void *y =sf_malloc(sizeof(int)); //allocate something afterward to extend heap and recreate wilderness

	cr_assert_not_null(x, "x is NULL!");
	cr_assert_not_null(y, "y is NULL!");

	assert_free_list_size(NUM_FREE_LISTS-1, 1); //ensure that the wilderness is reborn
	assert_free_block_count(PAGE_SZ-64, 1); //check its size

}

//Allocating more than a page and then testing multiple reallocations to a smaller size
Test(sf_memsuite_student, malloc_past_page_and_multiple_realloc, .init = sf_mem_init, .fini = sf_mem_fini) {

	void *x = sf_malloc(4088);
    void *z = sf_realloc(x, 2040);
    void *y = sf_malloc(2040);
    void *a = sf_realloc(y, 1016);

	cr_assert_not_null(z, "z is NULL!");
	cr_assert_not_null(a, "a is NULL!");

	sf_block *bp = (sf_block *)((char*)z - 2*sizeof(sf_header));
	cr_assert((bp -> header & BLOCK_SIZE_MASK) == 2048, "z is the wrong size!");
	bp = (sf_block *)((char*)a - 2*sizeof(sf_header));
	cr_assert((bp -> header & BLOCK_SIZE_MASK) == 1024, "a is the wrong size!");

	assert_free_list_size(NUM_FREE_LISTS-1, 1); //check that there aren't blocks freed unnecessarily or added to wilderness index incorrectly
	assert_free_block_count(4992, 1); //ensure its size

}

//Testing memalign with alignments that are a page and forces heap extension
Test(sf_memsuite_student, memalign_page, .init = sf_mem_init, .fini = sf_mem_fini) {

	void *s = sf_malloc(12352); //to save the initial pointer
	size_t init_addr = (size_t) s;
	sf_free(s);

	void *x = sf_memalign(4088, 8192);
	cr_assert_not_null(x, "x is NULL!");
	size_t addr = (size_t) x;
	cr_assert( addr%8192 == 0, "x is NOT ALIGNED!");

	if (init_addr%8192 == 0){
		cr_assert(s == x, "s is in the wrong position!");
		assert_free_list_size(NUM_FREE_LISTS-1, 1);
		assert_free_block_count(12160, 1); //only a free block afterwards
	} else {
		size_t first = 8192 - (init_addr%8192);
		assert_free_block_count(first, 1); //a free block before
		assert_free_list_size(NUM_FREE_LISTS-1, 1); //and the wilderness afterward

	}
}

//Testing memalign with alignments smaller than the given size and testing with the lowest base size
Test(sf_memsuite_student, memalign_smalls, .init = sf_mem_init, .fini = sf_mem_fini) {

	void *x = sf_memalign(1016, 64);
	cr_assert_not_null(x, "x is NULL!");
	size_t addr = (size_t) x;
	cr_assert( addr%64 == 0, "x is NOT ALIGNED!");

	sf_block *bp = (sf_block *)((char*)x - 2*sizeof(sf_header));
	cr_assert((bp -> header & BLOCK_SIZE_MASK) == 1024, "x is the wrong size!");

	assert_free_list_size(NUM_FREE_LISTS-1, 1); //check presence of wilderness
	assert_free_block_count(PAGE_SZ - 1024 - 48 - 64 - 16, 1); //check its size

}

