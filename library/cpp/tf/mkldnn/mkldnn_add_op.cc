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

/// CPU-dispatched kernel for "_MkldnnAdd" op. MKLDNN not used.

#if defined(ARCADIA_BUILD_ROOT)

#include "mkldnn_util.h"
#include <util/system/cpu_id.h>
#include <util/system/sanitizers.h>

namespace tensorflow {
    typedef Eigen::ThreadPoolDevice CPUDevice;

    template <typename T>
    __attribute__((always_inline)) static inline void DoAdd_ref(T* z, size_t N, const T* x, const T* y) {
        for (size_t n = 0; n < N; ++n) {
            z[n] = x[n] + y[n];
        }
    }

    template <typename... Args>
    __attribute__((target("avx2"))) static void DoAdd_AVX2(Args&&... args) {
        DoAdd_ref(args...);
    }

    template <typename... Args>
    __attribute__((target("avx"))) static void DoAdd_AVX(Args&&... args) {
        DoAdd_ref(args...);
    }

    template <typename T>
    static void DoAdd(T* z, size_t N, const T* x, const T* y) {
        if (NX86::CachedHaveAVX2()) {
            DoAdd_AVX2(z, N, x, y);
        } else if (NX86::CachedHaveAVX()) {
            DoAdd_AVX(z, N, x, y);
        } else {
            DoAdd_ref(z, N, x, y);
        }
        NSan::Unpoison(z, N * sizeof(*z));
    }

    // An implementation of Add (forward).
    template <typename Device, typename T>
    class MkldnnAddOp: public OpKernel {
    public:
        explicit MkldnnAddOp(OpKernelConstruction* context)
            : OpKernel(context)
        {
        }

        void Compute(OpKernelContext* context) override {
            const Tensor& x = MkldnnGetInput(context, 0);
            MkldnnShape x_mklshape;
            GetMkldnnShape(context, 0, &x_mklshape, mkldnn_any);

            const Tensor& y = MkldnnGetInput(context, 1);
            MkldnnShape y_mklshape;
            GetMkldnnShape(context, 1, &y_mklshape, mkldnn_any);

            CHECK(x_mklshape.IsMkldnnTensor() && y_mklshape.IsMkldnnTensor());
            CHECK(equal(x_mklshape.get_mpd(), y_mklshape.get_mpd()));

            MkldnnShape z_mklshape;
            z_mklshape.SetMkldnnTensor(true);
            z_mklshape.SetMkldnnLayout(x_mklshape.get_mpd());
            z_mklshape.SetTfLayout(x_mklshape.tf_mpd());

            Tensor* z = nullptr;
            AllocateOutputSetMkldnnShape(context, 0, &z, x.shape(), z_mklshape);

            const T* xdata = x.flat<T>().data();
            const T* ydata = y.flat<T>().data();
            T* zdata = z->flat<T>().data();
            size_t N = z->flat<T>().size();

            VLOG(1) << "MKLDNN: " << type_string() << " '" << name() << "'";

            DoAdd(zdata, N, xdata, ydata);
        }
    };

#define REGISTER_KERNEL_BUILDER_FOR(type)                                \
    REGISTER_KERNEL_BUILDER(Name("_MkldnnAdd")                           \
                                .Device(DEVICE_CPU)                      \
                                .TypeConstraint<type>("T")               \
                                .Label(mkl_op_registry::kMkldnnOpLabel), \
                            MkldnnAddOp<CPUDevice, type>)

    TF_CALL_float(REGISTER_KERNEL_BUILDER_FOR);

}
#endif /* ARCADIA_BUILD_ROOT */
