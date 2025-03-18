#include "rename.h"
#include "tensorflow/core/kernels/cwise_ops_common.h"

namespace tensorflow {

REGISTER_KERNEL_BUILDER(Name("Mul")
    .Device(DEVICE_CPU)
    .TypeConstraint<float>("T")
    .Label("avx")
    ,
    BinaryOp<CPUDevice, functor::mul<float>>);

}

extern "C" void RegisterCwiseOpMul1Avx() {}
