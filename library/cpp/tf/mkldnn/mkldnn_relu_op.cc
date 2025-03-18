/* Copyright 2017 The TensorFlow Authors. All Rights Reserved.
   Copyright 2017-2018 YANDEX LLC

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
==============================================================================*/

#if defined(ARCADIA_BUILD_ROOT)

#include "mkldnn_util.h"
#include <util/system/cpu_id.h>

namespace tensorflow {
    typedef Eigen::ThreadPoolDevice CPUDevice;

    template <typename T>
    __attribute__((always_inline)) static inline void DoRelu_ref(T* z, size_t N, const T* x, float leak) {
        const T zero = T(0);
        if (leak == 0) {
            for (size_t n = 0; n < N; ++n) {
                z[n] = x[n] > zero ? x[n] : zero;
            }
        } else {
            for (size_t n = 0; n < N; ++n) {
                z[n] = x[n] > zero ? x[n] : x[n] * leak;
            }
        }
    }

    template <typename... Args>
    __attribute__((target("avx2"))) static void DoRelu_AVX2(Args&&... args) {
        DoRelu_ref(args...);
    }

    template <typename... Args>
    __attribute__((target("avx"))) static void DoRelu_AVX(Args&&... args) {
        DoRelu_ref(args...);
    }

    template <typename T>
    static void DoRelu(T* z, size_t N, const T* x, float leak) {
        if (NX86::CachedHaveAVX2()) {
            DoRelu_AVX2(z, N, x, leak);
        } else if (NX86::CachedHaveAVX()) {
            DoRelu_AVX(z, N, x, leak);
        } else {
            DoRelu_ref(z, N, x, leak);
        }
    }

    // An implementation of Relu (forward).
    template <typename Device, typename T>
    class MkldnnReluOp: public OpKernel {
    public:
        explicit MkldnnReluOp(OpKernelConstruction* context)
            : OpKernel(context)
        {
            OP_REQUIRES_OK(context, context->GetAttr("leak", &leak_));
        }

        void Compute(OpKernelContext* context) override {
            const Tensor& x = MkldnnGetInput(context, 0);
            MkldnnShape x_mklshape;
            GetMkldnnShape(context, 0, &x_mklshape, mkldnn_any);

            MkldnnShape z_mklshape;
            z_mklshape.SetMkldnnTensor(x_mklshape.IsMkldnnTensor());
            z_mklshape.SetMkldnnLayout(x_mklshape.get_mpd());
            z_mklshape.SetTfLayout(x_mklshape.tf_mpd());

            Tensor* z = nullptr;
            AllocateOutputSetMkldnnShape(context, 0, &z, x.shape(), z_mklshape);

            const T* xdata = x.flat<T>().data();
            T* zdata = z->flat<T>().data();
            size_t N = z->flat<T>().size();

            VLOG(1) << "MKLDNN: " << type_string() << " '" << name() << "'"
                    << " leak=" << leak_;

            DoRelu(zdata, N, xdata, leak_);
        }

    private:
        float leak_;
    };

#define REGISTER_KERNEL_BUILDER_FOR(type)                                \
    REGISTER_KERNEL_BUILDER(Name("_MkldnnRelu")                          \
                                .Device(DEVICE_CPU)                      \
                                .TypeConstraint<type>("T")               \
                                .Label(mkl_op_registry::kMkldnnOpLabel), \
                            MkldnnReluOp<CPUDevice, type>)

    TF_CALL_float(REGISTER_KERNEL_BUILDER_FOR);

}
#endif /* ARCADIA_BUILD_ROOT */
