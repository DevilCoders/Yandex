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

#include <omp.h>

namespace tensorflow {
    typedef Eigen::ThreadPoolDevice CPUDevice;

    // The following few functions convert data from one to another mkldnn memory
    // format. It is ok to use them instead of mkldnn_reorder primitive,  because
    // (1) the functions are used only in the first invocation of the kernel
    // (results stored in persistent tensor), (2) they are written in a blocked
    // manner that compiler optimizes well (in fact, mkldnn reorder primitives are
    // such).

    // Convert data to mkldnn_Ohwi8o from mkldnn_hwio memory format
    template <typename T>
    static void Ohwi8o_for_hwio(T* out, const Mkldnn_md* out_md,
                                const T* in, const Mkldnn_md* in_md) {
        const int O = in_md->dims[0];
        const int I = in_md->dims[1];
        const int H = in_md->dims[2];
        const int W = in_md->dims[3];
        const int o_stride = in_md->layout_desc.blocking.strides[0][0];
        const int i_stride = in_md->layout_desc.blocking.strides[0][1];
        constexpr int B = 8;
        for (int ob = 0; ob < O / B; ++ob) {
            for (int h = 0; h < H; ++h) {
                for (int w = 0; w < W; ++w) {
                    const T* src = &in[in_md->blk_off(B * ob, 0, h, w)];
                    T* dst = &out[out_md->blk_off(1 * ob, 0, h, w)];
                    for (int i = 0; i < I; ++i) {
                        for (int o8 = 0; o8 < B; ++o8) {
                            dst[i * B + o8] = src[o8 * o_stride + i * i_stride];
                        }
                    }
                }
            }
        }
    }

    // Convert data to mkldnn_OIhw8i8o from mkldnn_hwio memory format
    template <typename T>
    static void OIhw8i8o_for_hwio(T* out, const Mkldnn_md* out_md,
                                  const T* in, const Mkldnn_md* in_md) {
        const int O = in_md->dims[0];
        const int I = in_md->dims[1];
        const int H = in_md->dims[2];
        const int W = in_md->dims[3];
        const int o_stride = in_md->layout_desc.blocking.strides[0][0];
        const int i_stride = in_md->layout_desc.blocking.strides[0][1];
        constexpr int B = 8;
        for (int ob = 0; ob < O / B; ++ob) {
            for (int ib = 0; ib < I / B; ++ib) {
                for (int h = 0; h < H; ++h) {
                    for (int w = 0; w < W; ++w) {
                        const T* src = &in[in_md->blk_off(B * ob, B * ib, h, w)];
                        T* dst = &out[out_md->blk_off(1 * ob, 1 * ib, h, w)];
                        for (int i8 = 0; i8 < B; ++i8) {
                            for (int o8 = 0; o8 < B; ++o8) {
                                dst[i8 * B + o8] = src[o8 * o_stride + i8 * i_stride];
                            }
                        }
                    }
                }
            }
        }
    }

    // Convert data to mkldnn_oihw from mkldnn_hwio memory format
    template <typename T>
    static void oihw_for_hwio(T* out, const Mkldnn_md* out_md,
                              const T* in, const Mkldnn_md* in_md) {
        const int O = in_md->dims[0];
        const int I = in_md->dims[1];
        const int H = in_md->dims[2];
        const int W = in_md->dims[3];
        const int o_stride = in_md->layout_desc.blocking.strides[0][0];
        const int i_stride = in_md->layout_desc.blocking.strides[0][1];
        const int o_stride_dst = out_md->layout_desc.blocking.strides[0][0];
        const int i_stride_dst = out_md->layout_desc.blocking.strides[0][1];
        for (int h = 0; h < H; ++h) {
            for (int w = 0; w < W; ++w) {
                const T* src = &in[in_md->blk_off(0, 0, h, w)];
                T* dst = &out[out_md->blk_off(0, 0, h, w)];
                for (int o = 0; o < O; ++o) {
                    for (int i = 0; i < I; ++i) {
                        dst[o * o_stride_dst + i * i_stride_dst] = src[o * o_stride + i * i_stride];
                    }
                }
            }
        }
    }

    // Convert data to mkldnn_gOIhw8i8o from mkldnn_ghwio memory format
    template <typename T>
    static void gOIhw8i8o_for_ghwio(T* out, const Mkldnn_md* out_md,
                                    const T* in, const Mkldnn_md* in_md) {
        const int G = in_md->dims[0];
        const int O = in_md->dims[1];
        const int I = in_md->dims[2];
        const int H = in_md->dims[3];
        const int W = in_md->dims[4];
        const int o_stride = in_md->layout_desc.blocking.strides[0][1];
        const int i_stride = in_md->layout_desc.blocking.strides[0][2];
        constexpr int B = 8;
        for (int g = 0; g < G; ++g) {
            for (int ob = 0; ob < O / B; ++ob) {
                for (int ib = 0; ib < I / B; ++ib) {
                    for (int h = 0; h < H; ++h) {
                        for (int w = 0; w < W; ++w) {
                            const T* src = &in[in_md->blk_off(g, B * ob, B * ib, h, w)];
                            T* dst = &out[out_md->blk_off(g, 1 * ob, 1 * ib, h, w)];
                            for (int i8 = 0; i8 < B; ++i8) {
                                for (int o8 = 0; o8 < B; ++o8) {
                                    dst[i8 * B + o8] = src[o8 * o_stride + i8 * i_stride];
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    template <typename Device, typename T, bool biasEnabled>
    class MkldnnConv2DOp: public OpKernel {
    public:
        ~MkldnnConv2DOp() override {
        }

        explicit MkldnnConv2DOp(OpKernelConstruction* context)
            : OpKernel(context)
        {
            OP_REQUIRES_OK(context, context->GetAttr("strides", &strides_));
            string data_format;
            OP_REQUIRES_OK(context, context->GetAttr("data_format", &data_format));
            OP_REQUIRES(context, FormatFromString(data_format, &data_format_),
                        errors::InvalidArgument("Invalid data format"));
            OP_REQUIRES(context, strides_.size() == 4,
                        errors::InvalidArgument("Sliding window strides field must "
                                                "specify 4 dimensions"));

            const int64 stride_n = GetTensorDim(strides_, data_format_, 'N');
            const int64 stride_c = GetTensorDim(strides_, data_format_, 'C');
            OP_REQUIRES(
                context, stride_n == 1 && stride_c == 1,
                errors::InvalidArgument("Current implementation does not yet support "
                                        "strides in the batch and depth dimensions."));
            OP_REQUIRES_OK(context, context->GetAttr("padding", &padding_));
            OP_REQUIRES_OK(context, context->GetAttr("leak", &leak_));
            OP_REQUIRES_OK(context, context->GetAttr("groups", &groups_));
            OP_REQUIRES_OK(context, context->GetAttr("use_weights_as_given", &MkldnnUseWeightsAsGiven_));
            if (context->HasAttr("openmp_threads")) {
                OP_REQUIRES_OK(context, context->GetAttr("openmp_threads", &OmpThreads_));
            }
        }

        void Compute(OpKernelContext* context) override {
            if (OmpThreads_ > 0) {
                omp_set_num_threads(OmpThreads_);
            }
            const Tensor* input = &MkldnnGetInput(context, 0);
            MkldnnShape input_shape;
            GetMkldnnShape(context, 0, &input_shape, data_format_ == FORMAT_NHWC ? mkldnn_nhwc : mkldnn_nchw);

            const Tensor* filter = &MkldnnGetInput(context, 1);
            MkldnnShape filter_shape;
            GetMkldnnShape(context, 1, &filter_shape, filter->dims() > 4 ? MkldnnMemoryFormat::mkldnn_ghwio : MkldnnMemoryFormat::mkldnn_hwio);

            const int filter_g = groups_ > 1 ? filter_shape.GetSize('G') : 1;
            OP_REQUIRES(context, groups_ == filter_g,
                        errors::InvalidArgument("bad filter size for groups=",
                                                groups_, ": ", filter_g, ", ",
                                                filter->DebugString()));

            const int filter_oc = filter_shape.GetSize('O');
            const int filter_ic = filter_shape.GetSize('I');
            const int filter_rows = filter_shape.GetSize('H');
            const int filter_cols = filter_shape.GetSize('W');

            const Tensor* bias = nullptr;
            if (biasEnabled) {
                bias = &MkldnnGetInput(context, 2);
                if (groups_ > 1) {
                    OP_REQUIRES(context, bias->dims() == 2,
                                errors::InvalidArgument("bias with groups must be 2-dimensional: ",
                                                        bias->shape().DebugString()));
                } else {
                    OP_REQUIRES(context, bias->dims() == 1 || bias->dims() == 2,
                                errors::InvalidArgument("bias must be 1- or 2-dimensional: ",
                                                        bias->shape().DebugString()));
                }
            }

            const int input_depth = input_shape.GetSize('C');
            OP_REQUIRES(
                context, input_depth / groups_ == filter_ic,
                errors::InvalidArgument("input and filter must have the same depth: ",
                                        input_depth / groups_, " vs ", filter_ic, " for ",
                                        filter->DebugString()));
            // The last dimension for filter is out_depth.
            const int out_depth = filter_oc * groups_;

            const int input_rows = input_shape.GetSize('H');
            const int input_cols = input_shape.GetSize('W');

            // The first dimension for input is batch.
            const int batch = input_shape.GetSize('N');

            // For now we take the stride from the second and third dimensions only (we
            // do not support striding on the batch or depth dimension).
            const int stride_rows = GetTensorDim(strides_, data_format_, 'H');
            const int stride_cols = GetTensorDim(strides_, data_format_, 'W');

            int out_rows, pad_rows, pad_rows_after;
            int out_cols, pad_cols, pad_cols_after;
            OP_REQUIRES_OK(context, GetWindowedOutputSizeVerbose_int(
                                        input_rows, filter_rows, stride_rows,
                                        padding_, &out_rows, &pad_rows, &pad_rows_after));
            OP_REQUIRES_OK(context, GetWindowedOutputSizeVerbose_int(
                                        input_cols, filter_cols, stride_cols,
                                        padding_, &out_cols, &pad_cols, &pad_cols_after));

            TensorShape out_shape =
                ShapeFromFormat(data_format_, batch, out_rows, out_cols, out_depth);

            // Output tensor is of the following dimensions:
            // [ in_batch, out_rows, out_cols, out_depth ]
            Tensor* output = nullptr;

            // If there is nothing to compute, return.
            if (out_shape.num_elements() == 0) {
                // TODO: Verify correctness here
                // Need semantics for Null MKL tensor
                return;
            }

            if (batch == 0) {
                // Nothing to do, allocate output tensor and return
                MkldnnShape mkl_output_mkl_shape;
                mkl_output_mkl_shape.SetMkldnnTensor(false);
                AllocateOutputSetMkldnnShape(context, 0, &output, input->shape(),
                                             mkl_output_mkl_shape);
                return;
            }

            CHECK(out_depth % groups_ == 0);
            CHECK(input_depth % groups_ == 0);
            mkldnn_dims_t input_nchw = {batch, input_depth, input_rows, input_cols};
            Mkldnn_md conv_filter_md;
            const auto filter_memory_format = [&] () {
               if (groups_ > 1) {
                   if (!MkldnnUseWeightsAsGiven_) {
                       return mkldnn_goihw;
                   } else {
                       CHECK(filter_ic % 8 == 0 && filter_oc % 8 == 0);
                       return mkldnn_gOIhw8i8o;
                   }
               } else {
                   if (!MkldnnUseWeightsAsGiven_) {
                       return mkldnn_oihw;
                   } else {
                       if (filter_oc % 8 != 0) {
                           return mkldnn_oihw;
                       } else if (filter_ic % 8 == 0) {
                           return mkldnn_OIhw8i8o;
                       } else {
                           return mkldnn_Ohwi8o;
                       }
                   }
               }
            } ();

            if (groups_ > 1) {
                mkldnn_dims_t filter_goihw = {groups_, out_depth / groups_, input_depth / groups_, filter_rows, filter_cols};
                conv_filter_md.Set(T(), 5, filter_goihw, filter_memory_format);
            } else {
                mkldnn_dims_t filter_oihw = {out_depth, input_depth, filter_rows, filter_cols};
                conv_filter_md.Set(T(), 4, filter_oihw, filter_memory_format);
            }
            mkldnn_dims_t out_nchw = {batch, out_depth, out_rows, out_cols};
            mkldnn_dims_t bias_x = {out_depth};

            // Create convolution primitive descriptor
            Mkldnn_md conv_src_md(T(), 4, input_nchw, mkldnn_any);
            Mkldnn_md conv_bias_md(T(), 1, bias_x, biasEnabled ? mkldnn_x : mkldnn_format_undef);
            Mkldnn_md conv_dst_md(T(), 4, out_nchw, mkldnn_any);
            mkldnn_dims_t conv_strides = {stride_rows, stride_cols};
            mkldnn_dims_t conv_padding_l = {pad_rows, pad_cols};
            mkldnn_dims_t conv_padding_r = {pad_rows_after, pad_cols_after};
            mkldnn_convolution_desc_t conv_desc;
            CK(mkldnn_convolution_forward_desc_init(&conv_desc,
                                                    mkldnn_forward_inference, mkldnn_convolution_direct,
                                                    &conv_src_md, &conv_filter_md, &conv_bias_md, &conv_dst_md,
                                                    conv_strides, conv_padding_l, conv_padding_r, mkldnn_padding_zero));

            Mkldnn_pd conv_pd;
            if (leak_ == 1) {
                CK(mkldnn_primitive_desc_create(&conv_pd, &conv_desc, Mkldnn::engine(), nullptr));
            } else {
                mkldnn_primitive_attr_t attr;
                CK(mkldnn_primitive_attr_create(&attr));
                mkldnn_post_ops_t post_ops;
                CK(mkldnn_post_ops_create(&post_ops));
                CK(mkldnn_post_ops_append_eltwise(post_ops, 1, mkldnn_eltwise_relu, leak_, 0));
                CK(mkldnn_primitive_attr_set_post_ops(attr, post_ops));
                CK(mkldnn_primitive_desc_create_v2(&conv_pd, &conv_desc, attr, Mkldnn::engine(), nullptr));

                CK(mkldnn_post_ops_destroy(post_ops));
                CK(mkldnn_primitive_attr_destroy(attr));
            }

            TensorShape mkl_output_tf_shape;
            MkldnnShape mkl_output_mkl_shape;
            mkl_output_mkl_shape.SetMkldnnTensor(true);
            mkl_output_mkl_shape.SetMkldnnLayout(conv_pd.get_mpd_at(mkldnn_query_dst_pd));

            Mkldnn_md out_tf_md;
            out_tf_md.Set(T(), 4, {batch, out_depth, out_rows, out_cols},
                          data_format_ == FORMAT_NCHW ? mkldnn_nchw : mkldnn_nhwc);
            Mkldnn_mpd out_tf_mpd(&out_tf_md);
            mkl_output_mkl_shape.SetTfLayout(out_tf_mpd);

            // MKL might change the dimension ordering
            // Create mapping to recover the original TF dimension order

            mkl_output_tf_shape.AddDim(mkl_output_mkl_shape.GetMemorySize() / sizeof(T));
            AllocateOutputSetMkldnnShape(context, 0, &output, mkl_output_tf_shape,
                                         mkl_output_mkl_shape);
            Mkldnn_memory out_mem(conv_pd.get_mpd_at(mkldnn_query_dst_pd), output->flat<T>().data());

            // Create the bias memory for mkldnn_conv.
            Mkldnn_memory bias_mem;
            if (biasEnabled) {
                Mkldnn_md bias_md(T(), 1, bias_x, mkldnn_x);
                Mkldnn_mpd bias_mpd(&bias_md);
                bias_mem.Set(bias_mpd, bias->flat<T>().data());
            }

            // Create the input primitive (with possible reorder) for mkldnn_conv
            Mkldnn_memory input_mem(input_shape.get_mpd(), input->flat<T>().data());
            Tensor mkl_tmp_input_buf_tensor;
            Mkldnn_memory conv_input_mem;
            Mkldnn_primitive reorder_input;
            const auto& input_mpd = input_mem.get_mpd();
            const auto& conv_input_mpd = conv_pd.get_mpd_at(mkldnn_query_src_pd);
            if (equal(input_mpd, conv_input_mpd)) {
                conv_input_mem.Set(input_mpd, input_mem.get_data_handle());
            } else {
                void* buf = nullptr;
                AllocTmpBuffer(context, &mkl_tmp_input_buf_tensor, conv_input_mpd, &buf);
                conv_input_mem.Set(conv_input_mpd, buf);

                Mkldnn_pd reorder_pd;
                CK(mkldnn_reorder_primitive_desc_create(&reorder_pd, input_mpd, conv_input_mpd));
                mkldnn_primitive_at_t inputs[] = {mkldnn_primitive_at(input_mem, 0)};
                const_mkldnn_primitive_t outputs[] = {conv_input_mem};
                CK(mkldnn_primitive_create(&reorder_input, reorder_pd, inputs, outputs));
            }

            // Create conv_filter_mem (with possible caching in the persistent tensor)

            const auto& filter_mpd = filter_shape.get_mpd();
            const auto& conv_filter_mpd = conv_pd.get_mpd_at(mkldnn_query_weights_pd);
            Mkldnn_memory filter_mem(filter_mpd, filter->flat<T>().data());
            Mkldnn_memory conv_filter_mem;
            if (MkldnnUseWeightsAsGiven_ || equal(filter_mpd, conv_filter_mpd)) {
                conv_filter_mem.Set(conv_filter_mpd, filter_mem.get_data_handle());
            } else {
                if (!weights_.AllocatedBytes()) {
                    const Mkldnn_md* f_memory_desc = GetMkldnnMd(filter_shape.get_mpd());
                    const auto& f_format = f_memory_desc->format;
                    OP_REQUIRES(context, f_format == mkldnn_oihw || f_format == mkldnn_goihw,
                                errors::InvalidArgument("unsupported mkldnn_memory_format for filter: ", f_format));
                    const T* f_data = filter->flat<T>().data();

                    mutex_lock l(mu_);
                    Tensor* weights_tensor;
                    OP_REQUIRES_OK(context, context->allocate_persistent(
                                                filter->dtype(), TensorShape({filter->NumElements()}),
                                                &weights_, &weights_tensor));

                    const Mkldnn_md* w_memory_desc = GetMkldnnMd(conv_filter_mpd);
                    const auto& w_format = w_memory_desc->format;
                    T* w_data = weights_tensor->flat<T>().data();

                    // Convert filter/f_format -> weights/w_format
                    switch (w_format) {
                        case mkldnn_Ohwi8o:
                            Ohwi8o_for_hwio(w_data, w_memory_desc, f_data, f_memory_desc);
                            break;
                        case mkldnn_oihw:
                            oihw_for_hwio(w_data, w_memory_desc, f_data, f_memory_desc);
                            break;
                        case mkldnn_OIhw8i8o:
                            OIhw8i8o_for_hwio(w_data, w_memory_desc, f_data, f_memory_desc);
                            break;
                        case mkldnn_gOIhw8i8o:
                            gOIhw8i8o_for_ghwio(w_data, w_memory_desc, f_data, f_memory_desc);
                            break;
                        default:
                            context->CtxFailure(errors::InvalidArgument("unsupported mkldnn_memory_format for weights: ", w_format));
                    }
                }
                conv_filter_mem.Set(conv_filter_mpd, weights_.AccessTensor(context)->template flat<T>().data());
            }

            mkldnn_primitive_at_t conv_srcs[] = {
                mkldnn_primitive_at(conv_input_mem, 0),
                mkldnn_primitive_at(conv_filter_mem, 0),
                mkldnn_primitive_at(bias_mem, 0),
            };

            const_mkldnn_primitive_t conv_dsts[] = {
                out_mem,
            };

            Mkldnn_primitive conv;
            CK(mkldnn_primitive_create(&conv, conv_pd, conv_srcs, conv_dsts));

            // Execute convolution
            mkldnn_primitive_t net[4];
            unsigned n = 0;
            if (reorder_input)
                net[n++] = reorder_input;
            net[n++] = conv;

            VLOG(1) << "MKLDNN: " << type_string() << " '" << name() << "'"
                    << (reorder_input ? " reorder_input" : "")
                    << " leak_=" << leak_
                    << " groups_=" << groups_
                    << " fmt=" << (data_format_ == FORMAT_NHWC ? "nhwc" : "nchw")
                    << " input_fmt=" << GetMkldnnMd(input_mpd)->format
                    << " conv_input_fmt=" << GetMkldnnMd(conv_input_mpd)->format
                    << " filter_fmt=" << GetMkldnnMd(filter_mpd)->format
                    << " conv_filter_fmt=" << GetMkldnnMd(conv_filter_mpd)->format
                    << " conv_output_fmt=" << GetMkldnnMd(conv_pd.get_mpd_at(mkldnn_query_output_pd))->format
                    ;

            Mkldnn_stream stream;
            CK(mkldnn_stream_submit(stream, n, net, NULL));
            CK(mkldnn_stream_wait(stream, n, NULL));
        }

        std::vector<int32> strides_;
        Padding padding_;
        TensorFormat data_format_;
        float leak_ = 1;   // 1 means no relu
        int32 groups_ = 1; // 1 means default convolution (with one group)
        mutex mu_;
        PersistentTensor weights_ GUARDED_BY(mu_);
        bool MkldnnUseWeightsAsGiven_; // Carries respective session option
        int OmpThreads_ = 0;
    };

    mkldnn_engine_t Mkldnn::the_engine;
    static struct Mkldnn_the_engine {
        Mkldnn_the_engine() {
            CK(mkldnn_engine_create(&Mkldnn::the_engine, mkldnn_cpu, 0));
        }
        ~Mkldnn_the_engine() {
            CK(mkldnn_engine_destroy(Mkldnn::the_engine));
        }
    } helper;

#define REGISTER_MKL_CPU(T)                                              \
    REGISTER_KERNEL_BUILDER(Name("_MkldnnConv2D")                        \
                                .Device(DEVICE_CPU)                      \
                                .TypeConstraint<T>("T")                  \
                                .Label(mkl_op_registry::kMkldnnOpLabel), \
                            MkldnnConv2DOp<CPUDevice, T, false>);        \
    REGISTER_KERNEL_BUILDER(Name("_MkldnnConv2DWithBias")                \
                                .Device(DEVICE_CPU)                      \
                                .TypeConstraint<T>("T")                  \
                                .Label(mkl_op_registry::kMkldnnOpLabel), \
                            MkldnnConv2DOp<CPUDevice, T, true>);

    TF_CALL_float(REGISTER_MKL_CPU);

}
#endif /* ARCADIA_BUILD_ROOT */
