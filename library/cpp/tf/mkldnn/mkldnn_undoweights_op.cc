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

#include "mkldnn_util.h"

namespace tensorflow {

    template <typename T>
    static void ConvertFromOIhw8i8oToHwio(const T* in, T* out, int64 H, int64 W, int64 I, int64 O) {
        // reshape(o//8, i//8, h, w, i8, o8).transpose(2, 3, 1, 4, 0, 5).reshape(h, w, i, o)
        for (int64 ob = 0; ob < O / 8; ++ob) {
            for (int64 ib = 0; ib < I / 8; ++ib) {
                for (int64 h = 0; h < H; ++h) {
                    for (int64 w = 0; w < W; ++w) {
                        for (int64 i8 = 0; i8 < 8; ++i8) {
                            for (int64 o8 = 0; o8 < 8; ++o8) {
                                out[h*W*I*O + w*I*O + ib*8*O + i8*O + ob*8 + o8] = in[ob*I*H*W*8 + ib*H*W*8*8 + h*W*8*8 + w*8*8 + i8*8 + o8];
                            }
                        }
                    }
                }
            }
        }
    }

    template <typename T>
    static void ConvertFromOhwi8oToHwio(const T* in, T* out, int64 H, int64 W, int64 I, int64 O) {
        // reshape(o//8, h, w, i, o8).transpose(1, 2, 3, 0, 4)
        for (int64 ob = 0; ob < O / 8; ++ob) {
            for (int64 h = 0; h < H; ++h) {
                for (int64 w = 0; w < W; ++w) {
                    for (int64 i = 0; i < I; ++i) {
                        for (int64 o8 = 0; o8 < 8; ++o8) {
                            out[h*W*I*O + w*I*O + i*O + ob*8 + o8] = in[ob*H*W*I*8 + h*W*I*8 + w*I*8 + i*8 + o8];
                        }
                    }
                }
            }
        }
    }

    template <typename Device, typename T>
    class UndoWeightsMkldnn: public OpKernel {
    public:
        explicit UndoWeightsMkldnn(OpKernelConstruction* context)
            : OpKernel(context)
        {
            OP_REQUIRES_OK(context, context->GetAttr("T", &data_type_));
        }

        void Compute(OpKernelContext* ctx) override {
            const Tensor& input_w = ctx->input(0);
            const TensorShape& hwio = input_w.shape();
            const int64 h = hwio.dim_size(0);
            const int64 w = hwio.dim_size(1);
            const int64 i = hwio.dim_size(2);
            const int64 o = hwio.dim_size(3);

            // refer to https://a.yandex-team.ru/arc//trunk/arcadia/cv/imgclassifiers/tf_applicator/example/danet.py?rev=3784272#L73
            if (o % 8 == 0 && i % 8 == 0) {
                // input is OIhw8i8o
                Tensor* output_w = nullptr;
                OP_REQUIRES_OK(ctx, ctx->allocate_output(0, hwio, &output_w));
                ConvertFromOIhw8i8oToHwio(input_w.flat<T>().data(), output_w->flat<T>().data(), h, w, i, o);

            } else if (o % 8 == 0) {
                // input is Ohwi8o
                Tensor* output_w = nullptr;
                OP_REQUIRES_OK(ctx, ctx->allocate_output(0, hwio, &output_w));
                ConvertFromOhwi8oToHwio(input_w.flat<T>().data(), output_w->flat<T>().data(), h, w, i, o);
            } else {
                ctx->set_output(0, input_w);
            }
        }

    private:
        DataType data_type_;
    };

    typedef Eigen::ThreadPoolDevice CPUDevice;

#define REGISTER_CPU(T) \
    REGISTER_KERNEL_BUILDER(Name("_UndoWeightsMkldnn") \
                            .Device(DEVICE_CPU) \
                            .TypeConstraint<T>("T"), \
                            UndoWeightsMkldnn<CPUDevice, T>);

    TF_CALL_float(REGISTER_CPU);
#undef REGISTER_CPU

}
