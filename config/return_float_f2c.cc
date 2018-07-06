#include <stdio.h>

#include "config.h"

#define BLAS_sdot FORTRAN_NAME( sdot, SDOT )

// returns *double*
#ifdef __cplusplus
extern "C"
#endif
double BLAS_sdot( const blas_int* n,
                  const float* x, const blas_int* incx,
                  const float* y, const blas_int* incy );

int main()
{
    blas_int n = 5, ione = 1;
    float x[] = { 1, 2, 3, 4, 5 };
    float y[] = { 5, 4, 3, 2, 1 };
    float result = BLAS_sdot( &n, x, &ione, y, &ione );
    bool okay = (result == 35);
    printf( "%s\n", okay ? "ok" : "failed" );
    return ! okay;
}