#include "rename.h"
#include "tensorflow/core/kernels/matmul_op.cc"

namespace tensorflow {
    REGISTER_KERNEL_BUILDER(Name("MatMul")
        .Device(DEVICE_CPU)
        .TypeConstraint<float>("T")
        .Label("avx")
        ,
        MatMulOp<CPUDevice, float, /*cublas=*/false>);
}

extern "C" void RegisterMatMulOpAvx() {}
