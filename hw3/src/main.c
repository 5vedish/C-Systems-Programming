#include <stdio.h>
#include "sfmm.h"

int main(int argc, char const *argv[]) {
    sf_mem_init();

    double* ptr = sf_malloc(sizeof(double));

    *ptr = 320320320e-320;
    //my stuff

    int* ptr2 = sf_malloc(127);
    *ptr2 = 69420;

    printf("%d", *ptr2);

    //my stuff

    printf("%.10e\n", *ptr);

    sf_free(ptr);

    return 0;

    sf_mem_fini();

    return EXIT_SUCCESS;
}
