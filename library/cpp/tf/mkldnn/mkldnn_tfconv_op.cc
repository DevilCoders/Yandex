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

namespace tensorflow {
    typedef Eigen::ThreadPoolDevice CPUDevice;

    ///////////////////////////////////////////////////////////
    //               Op kernel
    ///////////////////////////////////////////////////////////

    template <typename Device, typename T>
    class MkldnnToTfOp: public OpKernel {
    public:
        explicit MkldnnToTfOp(OpKernelConstruction* context)
            : OpKernel(context)
        {
            string data_format_str;
            OP_REQUIRES_OK(context, context->GetAttr("data_format", &data_format_str));
            OP_REQUIRES(context, FormatFromString(data_format_str, &data_format_),
                        errors::InvalidArgument("Invalid data format"));
            OP_REQUIRES_OK(context, context->GetAttr("T", &data_type_));
        }

        void Compute(OpKernelContext* context) override {
            // 1. Check that input tensor is in MKL format.
            const Tensor& input_tensor = MkldnnGetInput(context, 0);
            MkldnnShape input_shape;
            GetMkldnnShape(context, 0, &input_shape,
                           input_tensor.shape().dims() == 4 ? MkldnnMemoryFormatFor(data_format_) : mkldnn_any);

            // if input is already in Tf format, then just copy input tensor to output.
            if (!input_shape.IsMkldnnTensor()) {
                context->set_output(0, input_tensor);
                VLOG(1) << "MKLDNN: " << type_string() << " '" << name() << "'"
                        << " no conversion needed, set input as output";
                return;
            }

            // Check that input data type is same as operator data type and that it is
            // same as output data type.
            DataType input_data_type = input_type(0);
            DataType output_data_type = output_type(0);
            CHECK_EQ(data_type_, input_data_type);
            CHECK_EQ(data_type_, output_data_type);

            auto& input_mpd = input_shape.mkl_mpd();
            Mkldnn_memory input_mem(input_mpd, input_tensor.flat<T>().data());

            auto& out_mpd = input_shape.tf_mpd();
            const int64 N = out_mpd.GetSize('N');
            const int64 H = out_mpd.GetSize('H');
            const int64 W = out_mpd.GetSize('W');
            const int64 C = out_mpd.GetSize('C');
            TensorShape out_shape;
            if (data_format_ == FORMAT_NCHW) {
                out_shape = TensorShape({N, C, H, W});
            } else {
                out_shape = TensorShape({N, H, W, C});
            }

            // Allocate output tensor.
            Tensor* output_tensor = nullptr;

            if (equal(input_shape.mkl_mpd(), input_shape.tf_mpd())) {
                if (context->forward_input_to_output_with_shape(0, 0, out_shape, &output_tensor)) {
                    VLOG(1) << "MKLDNN: " << type_string() << " '" << name() << "'"
                            << " no conversion needed, forward input to output";
                    return;
                }
            }

            OP_REQUIRES_OK(context, context->allocate_output(0, out_shape, &output_tensor));
            Mkldnn_memory output_mem(out_mpd, output_tensor->flat<T>().data());

            Mkldnn_pd reorder_pd;
            CK(mkldnn_reorder_primitive_desc_create(&reorder_pd, input_mpd, out_mpd));

            Mkldnn_primitive reorder;
            mkldnn_primitive_at_t inputs[] = {mkldnn_primitive_at(input_mem, 0)};
            const_mkldnn_primitive_t outputs[] = {output_mem};
            CK(mkldnn_primitive_create(&reorder, reorder_pd, inputs, outputs));

            VLOG(1) << "MKLDNN: " << type_string() << " '" << name() << "'"
                    << " from " << mkldnn_primitive_desc_query_memory_d(input_mpd)->format
                    << " to " << mkldnn_primitive_desc_query_memory_d(out_mpd)->format
                    << " conversion executed";

            Mkldnn_stream stream;
            CK(mkldnn_stream_submit(stream, 1, &reorder, NULL));
            CK(mkldnn_stream_wait(stream, 1, NULL));
        }

    private:
        TensorFormat data_format_ = FORMAT_NHWC;
        DataType data_type_;
    };

    ///////////////////////////////////////////////////////////
    //               Register kernel
    ///////////////////////////////////////////////////////////

#define REGISTER_CPU(T)                                                  \
    REGISTER_KERNEL_BUILDER(Name("_MkldnnToTf")                          \
                                .Device(DEVICE_CPU)                      \
                                .TypeConstraint<T>("T")                  \
                                .Label(mkl_op_registry::kMkldnnOpLabel), \
                            MkldnnToTfOp<CPUDevice, T>);

    TF_CALL_float(REGISTER_CPU);
#undef REGISTER_CPU

}
#endif /* ARCADIA_BUILD_ROOT */
