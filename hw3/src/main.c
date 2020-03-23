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

    void* p = sf_malloc(56);
    void* p2 = sf_realloc(p, 128);
    

    
    printf("%p%p", p, p2);

    

    sf_show_heap();

    

    
   

    

    return EXIT_SUCCESS;
}
