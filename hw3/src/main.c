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

    double* ptr = sf_malloc(5000);
    double* ptr2 = sf_malloc(8);
    double* ptr3 = sf_malloc(8);

    *ptr = 1;
    *ptr2 = 2;
    *ptr3 = 3;
    printf("%f\n", *ptr);
    printf("%f\n", *ptr2);
    printf("%f\n", *ptr3);

    sf_show_heap();
   

    

    return EXIT_SUCCESS;
}
