#include "rename.h"
#include "tensorflow/core/kernels/bias_op.cc"

namespace tensorflow {
    REGISTER_KERNEL_BUILDER(Name("BiasAdd")
        .Device(DEVICE_CPU)
        .TypeConstraint<float>("T")
        .Label("avx")
        ,
        BiasOp<CPUDevice, float>);
}

extern "C" void RegisterBiasOpAvx() {}
