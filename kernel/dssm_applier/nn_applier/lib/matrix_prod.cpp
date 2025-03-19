#include "matrix_prod.h"

#include <library/cpp/dot_product/dot_product.h>

#include <util/generic/mem_copy.h>

namespace NNeuralNetApplier {

void MatrixProduct(const TMatrix& input, const TMatrix& transform, const TMatrix& bias, TMatrix* output) {
    for (size_t row = 0; row < input.GetNumRows(); ++row) {
        MemCopy((*output)[row], bias[0], bias.GetNumColumns());

        for (size_t transformRow = 0; transformRow < transform.GetNumRows(); ++transformRow) {
            (*output)[row][transformRow] += DotProduct(
                input[row], transform[transformRow], transform.GetNumColumns());
        }
    }
}

}
