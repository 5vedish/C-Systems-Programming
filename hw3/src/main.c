#include <stdio.h>
#include "sfmm.h"

// int main(int argc, char const *argv[]) {
//     sf_mem_init();

//     double* ptr = sf_malloc(sizeof(double));

//     *ptr = 320320320e-320;

//     printf("%.10e\n", *ptr);

//     sf_free(ptr);

//     sf_mem_fini();

//     return EXIT_SUCCESS;
// }

int main(int argc, char const *argv[]) {
    sf_mem_init();

    // void* p = sf_malloc(56);
    // void* p2 = sf_realloc(p, 128);
    

    sf_malloc(65407);
    sf_malloc(120);

    

    sf_show_heap();

    // void *x = sf_malloc(sizeof(int) * 20);
	// void *y = sf_realloc(x, sizeof(int) * 16);




    

    
   

    

    return EXIT_SUCCESS;
}
