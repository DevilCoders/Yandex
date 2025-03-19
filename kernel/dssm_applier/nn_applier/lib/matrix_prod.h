#pragma once

#include "states.h"

namespace NNeuralNetApplier {

void MatrixProduct(const TMatrix& input, const TMatrix& transform, const TMatrix& bias, TMatrix* output);

}
