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

    // double* ptr = sf_malloc(3904);
    // double* ptr2 = sf_malloc(5000);
    // double* ptr3 = sf_malloc(8);

    // *ptr = 1;
    // *ptr2 = 2;
    // *ptr3 = 3;
    // printf("%f\n", *ptr);
    // printf("%f\n", *ptr2);
    // printf("%f\n", *ptr3);

    // sf_free(ptr);
    // sf_free(ptr3);

    void *u = sf_malloc(200);
	/* void *v = */ sf_malloc(300);
	void *w = sf_malloc(200);
	/* void *x = */ sf_malloc(500);
	void *y = sf_malloc(200);
	/* void *z = */ sf_malloc(700);

	sf_free(u);
	sf_free(w);
	sf_free(y);

    // double* ptr4 = sf_malloc(15000);
    // *ptr4 = 100;

    sf_show_heap();

    // printf("%f", *ptr4);
   

    

    return EXIT_SUCCESS;
}
