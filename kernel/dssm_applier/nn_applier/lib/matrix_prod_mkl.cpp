#include "matrix_prod.h"

#include <contrib/libs/intel/mkl/include/mkl.h>

#include <util/generic/mem_copy.h>

namespace NNeuralNetApplier {

void MatrixProduct(const TMatrix& input, const TMatrix& transform, const TMatrix& bias, TMatrix* output) {
    if (mkl_cbwr_get(MKL_CBWR_BRANCH) == MKL_CBWR_BRANCH_OFF) {
        mkl_cbwr_set(MKL_CBWR_AUTO); // guaranteed to succeed; protection against alignment and cache sizes; more details in https://clubs.at.yandex-team.ru/arcadia/13851
    }

    for (size_t row = 0; row < input.GetNumRows(); ++row) {
        MemCopy((*output)[row], bias[0], bias.GetNumColumns());
    }

    const int savedThreadCount = mkl_set_num_threads_local(1);

    cblas_sgemm(/*matrix layout*/ CblasRowMajor, /*do not transpose input*/ CblasNoTrans, /*transpose transform*/ CblasTrans,
        input.GetNumRows(), transform.GetNumRows(), input.GetNumColumns(),
        /*alpha*/ 1.0f, input.GetData(), input.GetNumColumns(), transform.GetData(), transform.GetNumColumns(),
        /*beta*/ 1.0f, output->GetData(), output->GetNumColumns());

    mkl_set_num_threads_local(savedThreadCount);
}

}
