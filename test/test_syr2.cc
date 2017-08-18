#include <omp.h>

#include "test.hh"
#include "cblas.hh"
#include "lapack.hh"
#include "flops.hh"
#include "check_gemm.hh"

#include "syr2.hh"

// -----------------------------------------------------------------------------
template< typename TA, typename TX, typename TY >
void test_syr2_work( Params& params, bool run )
{
    using namespace blas;
    typedef typename traits3< TA, TX, TY >::scalar_t scalar_t;
    typedef typename traits< scalar_t >::norm_t norm_t;
    typedef long long lld;

    // get & mark input values
    blas::Layout layout = params.layout.value();
    blas::Uplo uplo = params.uplo.value();
    scalar_t alpha  = params.alpha.value();
    int64_t n       = params.dim.n();
    int64_t incx    = params.incx.value();
    int64_t incy    = params.incy.value();
    int64_t align   = params.align.value();
    int64_t verbose = params.verbose.value();

    // mark non-standard output values
    params.ref_time.value();
    params.ref_gflops.value();

    if ( ! run)
        return;

    // constants
    scalar_t one = 1;

    // setup
    int64_t lda = roundup( n, align );
    size_t size_A = size_t(lda)*n;
    size_t size_x = (n - 1) * abs(incx) + 1;
    size_t size_y = (n - 1) * abs(incy) + 1;
    TA* A    = new TA[ size_A ];
    TA* Aref = new TA[ size_A ];
    TX* x    = new TX[ size_x ];
    TY* y    = new TY[ size_y ];

    int64_t idist = 1;
    int iseed[4] = { 0, 0, 0, 1 };
    lapack_larnv( idist, iseed, size_A, A );
    lapack_larnv( idist, iseed, size_x, x );
    lapack_larnv( idist, iseed, size_y, y );
    lapack_lacpy( "g", n, n, A, lda, Aref, lda );

    // norms for error check
    norm_t work[1];
    norm_t Anorm = lapack_lansy( "f", uplo2str(uplo), n, A, lda, work );
    norm_t Xnorm = cblas_nrm2( n, x, abs(incx) );
    norm_t Ynorm = cblas_nrm2( n, y, abs(incy) );

    if (verbose >= 1) {
        printf( "A n=%5lld, lda=%5lld, size=%5lld, norm=%.2e\n"
                "x n=%5lld, inc=%5lld, size=%5lld, norm=%.2e\n"
                "y n=%5lld, inc=%5lld, size=%5lld, norm=%.2e\n",
                (lld) n, (lld) lda,  (lld) size_A, Anorm,
                (lld) n, (lld) incx, (lld) size_x, Xnorm,
                (lld) n, (lld) incy, (lld) size_y, Ynorm );
    }
    if (verbose >= 2) {
        printf( "A = " ); //print_matrix( n, n, A, lda );
        printf( "Aref = " ); //print_matrix( n, n, Aref, lda );
        printf( "x = " ); //print_vector( n, x, abs(incx) );
        printf( "y = " ); //print_vector( n, y, abs(incy) );
        printf( "alpha = %.4f + %.4fi;\n", real(alpha), imag(alpha) );
    }

    // run test
    libtest::flush_cache( params.cache.value() );
    double time = omp_get_wtime();
    blas::syr2( layout, uplo, n, alpha, x, incx, y, incy, A, lda );
    time = omp_get_wtime() - time;

    double gflop = gflop_syr2( n, A );
    params.time.value()   = time * 1000;  // msec
    params.gflops.value() = gflop / time;

    if (verbose >= 2) {
        printf( "A2 = " ); //print_matrix( n, n, A, lda );
    }

    if (params.check.value() == 'y') {
        // there are no csyr2/zsyr2, so use csyr2k/zsyr2k
        // needs XX, YY as matrices instead of vectors with stride.
        TX *XX = new TX[ lda ];
        TY *YY = new TY[ lda ];
        cblas_copy( n, x, incx, XX, 1 );
        cblas_copy( n, y, incy, YY, 1 );
        if (verbose >= 2) {
            printf( "XX = " ); //print_matrix( n, 1, XX, lda );
            printf( "YY = " ); //print_matrix( n, 1, YY, lda );
        }

        // run reference
        libtest::flush_cache( params.cache.value() );
        time = omp_get_wtime();

        // MacOS Cblas has bug with RowMajor [sd]syr2k???
        if (layout == Layout::RowMajor) {
            layout = Layout::ColMajor;
            uplo = (uplo == Uplo::Upper ? Uplo::Lower : Uplo::Upper);
        }
        cblas_syr2k( cblas_layout_const(layout), cblas_uplo_const(uplo), CblasNoTrans,
                     n, 1, alpha, XX, lda, YY, lda, one, Aref, lda );
        time = omp_get_wtime() - time;

        params.ref_time.value()   = time * 1000;  // msec
        params.ref_gflops.value() = gflop / time;

        if (verbose >= 2) {
            printf( "Aref = " ); //print_matrix( n, n, Aref, lda );
        }

        // check error compared to reference
        // beta = 1
        norm_t error;
        int64_t okay;
        check_herk( uplo, n, 2, alpha, scalar_t(1), Xnorm, Ynorm, Anorm,
                    Aref, lda, A, lda, &error, &okay );
        params.error.value() = error;
        params.okay.value() = okay;

        delete[] XX;
        delete[] YY;
    }

    delete[] A;
    delete[] Aref;
    delete[] x;
    delete[] y;
}

// -----------------------------------------------------------------------------
void test_syr2( Params& params, bool run )
{
    switch (params.datatype.value()) {
        case libtest::DataType::Integer:
            //test_syr2_work< int64_t >( params, run );
            throw std::exception();
            break;

        case libtest::DataType::Single:
            test_syr2_work< float, float, float >( params, run );
            break;

        case libtest::DataType::Double:
            test_syr2_work< double, double, double >( params, run );
            break;

        case libtest::DataType::SingleComplex:
            test_syr2_work< std::complex<float>, std::complex<float>,
                            std::complex<float> >( params, run );
            break;

        case libtest::DataType::DoubleComplex:
            test_syr2_work< std::complex<double>, std::complex<double>,
                            std::complex<double> >( params, run );
            break;
    }
}