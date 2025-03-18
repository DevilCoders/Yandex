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

    // An implementation of MaxPooling (forward).
    template <typename Device, typename T>
    class MkldnnMaxPoolingOp: public OpKernel {
    public:
        explicit MkldnnMaxPoolingOp(OpKernelConstruction* context)
            : OpKernel(context)
        {
            string data_format;

            OP_REQUIRES_OK(context, context->GetAttr("data_format", &data_format));
            OP_REQUIRES(context, FormatFromString(data_format, &data_format_),
                        errors::InvalidArgument("Invalid data format"));
            OP_REQUIRES_OK(context, context->GetAttr("ksize", &ksize_));
            OP_REQUIRES(context, ksize_.size() == 4,
                        errors::InvalidArgument("Sliding window ksize field must "
                                                "specify 4 dimensions"));
            OP_REQUIRES_OK(context, context->GetAttr("strides", &stride_));
            OP_REQUIRES(context, stride_.size() == 4,
                        errors::InvalidArgument("Sliding window stride field must "
                                                "specify 4 dimensions"));
            OP_REQUIRES_OK(context, context->GetAttr("padding", &padding_));
            OP_REQUIRES(context, ksize_[0] == 1 && stride_[0] == 1,
                        errors::Unimplemented("Pooling is not yet supported on the "
                                              "batch dimension."));
        }

        void Compute(OpKernelContext* context) override {
            const Tensor* input = &MkldnnGetInput(context, 0);
            MkldnnShape input_shape;
            GetMkldnnShape(context, 0, &input_shape, MkldnnMemoryFormatFor(data_format_));

            int batch = static_cast<int>(input_shape.GetSize('N'));
            int input_rows = static_cast<int>(input_shape.GetSize('H'));
            int input_cols = static_cast<int>(input_shape.GetSize('W'));
            int input_depth = static_cast<int>(input_shape.GetSize('C'));
            int kernel_rows = GetTensorDim(ksize_, data_format_, 'H');
            int kernel_cols = GetTensorDim(ksize_, data_format_, 'W');
            CHECK(GetTensorDim(ksize_, data_format_, 'C') == 1);
            int stride_rows = GetTensorDim(stride_, data_format_, 'H');
            int stride_cols = GetTensorDim(stride_, data_format_, 'W');
            CHECK(GetTensorDim(stride_, data_format_, 'C') == 1);
            int out_depth = input_depth;
            int out_rows, pad_rows, pad_rows_after;
            int out_cols, pad_cols, pad_cols_after;
            OP_REQUIRES_OK(context, GetWindowedOutputSizeVerbose_int(
                                        input_rows, kernel_rows, stride_rows, padding_,
                                        &out_rows, &pad_rows, &pad_rows_after));
            OP_REQUIRES_OK(context, GetWindowedOutputSizeVerbose_int(
                                        input_cols, kernel_cols, stride_cols, padding_,
                                        &out_cols, &pad_cols, &pad_cols_after));

            Mkldnn_md maxpooling_input_md;
            Mkldnn_md maxpooling_out_md;
            Mkldnn_pd maxpooling_pd;
            mkldnn_dims_t strides = {stride_rows, stride_cols};
            mkldnn_dims_t ksizes = {kernel_rows, kernel_cols};
            mkldnn_dims_t padding_l = {pad_rows, pad_cols};
            mkldnn_dims_t padding_r = {pad_rows_after, pad_cols_after};
            mkldnn_memory_format_t formats[] = {
                mkldnn_any,
                mkldnn_nChw8c,
                data_format_ == FORMAT_NCHW ? mkldnn_nchw : mkldnn_nhwc};
            mkldnn_status_t maxpooling_pd_create_call;
            for (auto fmt : formats) {
                maxpooling_input_md.Set(T(), 4, {batch, input_depth, input_rows, input_cols}, fmt);
                maxpooling_out_md.Set(T(), 4, {batch, out_depth, out_rows, out_cols}, fmt);
                mkldnn_pooling_desc_t maxpooling_desc;
                CK(mkldnn_pooling_forward_desc_init(&maxpooling_desc,
                                                    mkldnn_forward_inference, mkldnn_pooling_max,
                                                    &maxpooling_input_md, &maxpooling_out_md,
                                                    strides, ksizes, padding_l, padding_r,
                                                    mkldnn_padding_zero));
                maxpooling_pd_create_call = mkldnn_primitive_desc_create(&maxpooling_pd, &maxpooling_desc, Mkldnn::engine(), nullptr);
                if (maxpooling_pd_create_call == mkldnn_success)
                    break;
            }
            CK(maxpooling_pd_create_call);

            Tensor* output = nullptr;
            TensorShape out_shape =
                ShapeFromFormat(data_format_, batch, out_rows, out_cols, out_depth);

            if (out_shape.num_elements() == 0) {
                return;
            }

            if (batch == 0) {
                // Nothing to do, allocate output tensor and return
                MkldnnShape out_mklshape;
                out_mklshape.SetMkldnnTensor(false);
                AllocateOutputSetMkldnnShape(context, 0, &output, input->shape(), out_mklshape);
                return;
            }

            MkldnnShape out_mklshape;
            out_mklshape.SetMkldnnTensor(true);
            const auto& maxpooling_out_mpd = maxpooling_pd.get_mpd_at(mkldnn_query_dst_pd);
            out_mklshape.SetMkldnnLayout(maxpooling_out_mpd);
            Mkldnn_md out_md;
            out_md.Set(T(), 4, {batch, out_depth, out_rows, out_cols},
                       data_format_ == FORMAT_NCHW ? mkldnn_nchw : mkldnn_nhwc);
            Mkldnn_mpd out_mpd(&out_md);
            out_mklshape.SetTfLayout(out_mpd);

            AllocateOutputSetMkldnnShape(context, 0, &output, out_shape, out_mklshape);
            Mkldnn_memory out_mem(maxpooling_out_mpd, output->flat<T>().data());

            Mkldnn_memory input_mem(input_shape.get_mpd(), input->flat<T>().data());

            const auto& input_mpd = input_mem.get_mpd();
            const auto& maxpooling_input_mpd = maxpooling_pd.get_mpd_at(mkldnn_query_src_pd);
            Mkldnn_memory maxpooling_input_mem;
            Mkldnn_primitive reorder_input;
            Tensor buf_tensor;
            if (equal(input_mpd, maxpooling_input_mpd)) {
                maxpooling_input_mem.Set(input_mpd, input_mem.get_data_handle());
            } else {
                void* buf = nullptr;
                AllocTmpBuffer(context, &buf_tensor, maxpooling_input_mpd, &buf);
                maxpooling_input_mem.Set(maxpooling_input_mpd, buf);

                Mkldnn_pd reorder_pd;
                CK(mkldnn_reorder_primitive_desc_create(&reorder_pd, input_mpd, maxpooling_input_mpd));
                mkldnn_primitive_at_t inputs[] = {mkldnn_primitive_at(input_mem, 0)};
                const_mkldnn_primitive_t outputs[] = {maxpooling_input_mem};
                CK(mkldnn_primitive_create(&reorder_input, reorder_pd, inputs, outputs));
            }

            mkldnn_primitive_at_t maxpooling_inputs[] = {
                mkldnn_primitive_at(maxpooling_input_mem, 0),
            };

            const_mkldnn_primitive_t maxpooling_outputs[] = {
                out_mem,
            };

            Mkldnn_primitive maxpooling;
            CK(mkldnn_primitive_create(&maxpooling, maxpooling_pd, maxpooling_inputs, maxpooling_outputs));

            // Execute maxpooling
            mkldnn_primitive_t net[4];
            unsigned n = 0;
            if (reorder_input)
                net[n++] = reorder_input;
            net[n++] = maxpooling;

            VLOG(1) << "MKLDNN: " << type_string() << " '" << name() << "'"
                    << (reorder_input ? " reorder_input" : "")
                    << " padding_l=" << padding_l[0] << "," << padding_l[1]
                    << " padding_r=" << padding_r[0] << "," << padding_r[1];

            Mkldnn_stream stream;
            CK(mkldnn_stream_submit(stream, n, net, NULL));
            CK(mkldnn_stream_wait(stream, n, NULL));
        }

    private:
        std::vector<int32> ksize_;
        std::vector<int32> stride_;
        Padding padding_;
        TensorFormat data_format_;
        bool workspace_enabled_;
    };

    // TODO: The operation to compute MaxPool gradients.

    REGISTER_KERNEL_BUILDER(Name("_MkldnnMaxPool")
                                .Device(DEVICE_CPU)
                                .TypeConstraint<float>("T")
                                .Label(mkl_op_registry::kMkldnnOpLabel),
                            MkldnnMaxPoolingOp<CPUDevice, float>);

}
#endif /* ARCADIA_BUILD_ROOT */
