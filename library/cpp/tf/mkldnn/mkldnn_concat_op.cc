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
#include "tensorflow/core/kernels/concat_lib.h"
#include "contrib/deprecated/onednn/include/mkldnn_debug.h"

namespace tensorflow {

typedef Eigen::ThreadPoolDevice CPUDevice;

enum AxisArgumentName { NAME_IS_AXIS, NAME_IS_CONCAT_DIM };

std::tuple<int, mkldnn_memory_format_t> CalcOutputRankAndPreferredInputMemoryFormat(const MkldnnShapeList& input_shapes) {
    int output_rank = 0;
    mkldnn_memory_format_t preferred_memory_format = mkldnn_format_undef;
    for (const auto& input_shape : input_shapes) {
        // Scan mkldnn inputs first
        if (input_shape.IsMkldnnTensor()) {
            const mkldnn_memory_desc_t* memory_desc = mkldnn_primitive_desc_query_memory_d(input_shape.get_mpd());
            const mkldnn_memory_format_t memory_format = memory_desc->format;
            if (output_rank == 0) output_rank = memory_desc->ndims;
            if (preferred_memory_format == mkldnn_format_undef) {
                preferred_memory_format = memory_format;
            } else if (int(memory_format) >= int(preferred_memory_format)) {
                // nchw vs nChw8c => nChw8c, because nChw8c is more specific
                preferred_memory_format = memory_format;
            }
        }
    }
    // Check we have a good preferred_memory_format...
    if (preferred_memory_format == mkldnn_format_undef) {
        for (const auto& input_shape : input_shapes) {
            // ...otherwise scan non-mkldnn inputs
            if (!input_shape.IsMkldnnTensor()) {
                const mkldnn_memory_desc_t* memory_desc = mkldnn_primitive_desc_query_memory_d(input_shape.get_mpd());
                const mkldnn_memory_format_t memory_format = memory_desc->format;
                if (output_rank == 0) output_rank = memory_desc->ndims;
                if (preferred_memory_format == mkldnn_format_undef) {
                    preferred_memory_format = memory_format;
                    break;
                }
            }
        }
    }
    return {output_rank, preferred_memory_format};
}

// --------------------------------------------------------------------------
//                      Mkldnn Concat Op
// --------------------------------------------------------------------------

template <typename Device, typename T, AxisArgumentName AxisArgName>
class MkldnnConcatOp : public OpKernel {

 public:
    typedef std::vector<std::unique_ptr<typename TTypes<T, 2>::ConstMatrix>>
        ConstMatrixVector;

    explicit MkldnnConcatOp(OpKernelConstruction* context) : OpKernel(context) {
        string data_format;
        OP_REQUIRES_OK(context, context->GetAttr("data_format", &data_format));
        OP_REQUIRES(context, FormatFromString(data_format, &data_format_),
                    errors::InvalidArgument("Invalid data format"));
    }

    void Compute(OpKernelContext* context) override {

        // Get input tensors.
        OpInputList input_tensors;
        GetMkldnnInputList(context, "values", &input_tensors);
        const int numInputs = input_tensors.size();

        // Get MKL shapes.
        MkldnnShapeList input_shapes(numInputs);
        GetMkldnnShapeList(context, "values", &input_shapes);

        // Fixup input_shapes; TODO(dbakshee): move GuessTfLayout to GetMkldnnShapeList
        for (int i = 0; i < numInputs; ++i) {
            if (input_shapes[i].IsMkldnnTensor()) {
                ; // Good
            } else {
                Mkldnn_md md = GuessTfLayout(context, i, data_format_ == FORMAT_NHWC ? mkldnn_nhwc : mkldnn_any);
                Mkldnn_mpd mpd(&md);
                input_shapes[i].SetTfLayout(mpd);
            }
        }

        // If this is Concat, then concat_dim is 0th input.
        // If this is ConcatV2, then axis is Nth input.
        const Tensor& concat_dim_tensor = AxisArgName == NAME_IS_CONCAT_DIM
            ? MkldnnGetInput(context, 0)
            : MkldnnGetInput(context, numInputs);

        // Sanity checks
        OP_REQUIRES(context, IsLegacyScalar(concat_dim_tensor.shape()),
            errors::InvalidArgument(
                "Concat dim tensor should be a scalar integer, but got shape ",
                concat_dim_tensor.shape().DebugString()));
        int32 concat_dim = internal::SubtleMustCopy(concat_dim_tensor.scalar<int32>()());

        // Obtain preferred input memory format and output_rank. Ideally the
        // input memory format should be equal to the output memory format and
        // be an mkldnn format.
        int output_rank = 0;
        mkldnn_memory_format_t preferred_memory_format = mkldnn_format_undef;
        std::tie(output_rank, preferred_memory_format) = CalcOutputRankAndPreferredInputMemoryFormat(input_shapes);
        OP_REQUIRES(context, preferred_memory_format != mkldnn_format_undef,
            errors::InvalidArgument(type_string(),
                ": cannot determine preferred_memory_format for operation ", name()));
        if (concat_dim < 0) {
            concat_dim = output_rank + concat_dim;
        }

        // `concat_dim` is the number of the TF Tensor dimension to use.
        // For example, if we concat channels, then concat_dim=1 for NCHW and concat_dim=3 for NHWC.
        // `mkldnn_concat_dim` is the number of MKLDNN dimension to use. MKLDNN always uses NCHW order.
        // For example, channel dimension is always 1.
        // See https://a.yandex-team.ru/arc/trunk/arcadia/contrib/libs/intel/mkldnn/include/mkldnn_types.h?rev=3650162#L575-577

        OP_REQUIRES(context, output_rank == 4, errors::InvalidArgument("expected rank 4, got ", output_rank));
        int mkldnn_concat_dim;
        if (data_format_ == FORMAT_NHWC) {
            constexpr int mkldnn_for_tf[] = {0, 2, 3, 1};
            mkldnn_concat_dim = mkldnn_for_tf[concat_dim];
        } else {
            mkldnn_concat_dim = concat_dim;
        }

        // Prepare input primitives and optional reorder primitives, compute output_dims.
        struct TReorderPath {
            const mkldnn_memory_desc_t* input_md;
            const_mkldnn_primitive_desc_t input_mpd;

            Mkldnn_md reorder_md;
            Mkldnn_mpd reorder_mpd;
            void* buf = nullptr;
            Tensor tmp_tensor;
            Mkldnn_memory input_mem;
            Mkldnn_memory reorder_mem;
            Mkldnn_pd reorder_pd;
            Mkldnn_primitive reorder;
        };
        std::vector<TReorderPath> reorderPath(numInputs);
        mkldnn_dims_t output_dims;
        {
            for (int i = 0; i < numInputs; ++i) {
                reorderPath[i].input_mpd = input_shapes[i].get_mpd();
                reorderPath[i].input_md = mkldnn_primitive_desc_query_memory_d(reorderPath[i].input_mpd);
                reorderPath[i].input_mem.Set(reorderPath[i].input_mpd, input_tensors[i].flat<T>().data());
                OP_REQUIRES(context, reorderPath[i].input_md->ndims == output_rank,
                    errors::InvalidArgument(type_string(), ": Rank mismatch: ",
                        " expected ", output_rank, ", got ", reorderPath[i].input_md->ndims,
                        " for input ", i, " to operation ", name()));
                if (reorderPath[i].input_md->format != preferred_memory_format) {
                    // Need reordering, so far we can only do this for rank 4
                    OP_REQUIRES(context, output_rank == 4, errors::InvalidArgument(type_string(),
                        ": Unsupported rank: ", output_rank, " for operation ", name()));
                    const int batch = input_shapes[i].GetSize('N');
                    const int depth = input_shapes[i].GetSize('C');
                    const int rows = input_shapes[i].GetSize('H');
                    const int cols = input_shapes[i].GetSize('W');
                    reorderPath[i].reorder_md.Set(T(), 4, {batch, depth, rows, cols}, preferred_memory_format);
                    reorderPath[i].reorder_mpd.Set(&reorderPath[i].reorder_md);

                    AllocTmpBuffer(context, &reorderPath[i].tmp_tensor, reorderPath[i].reorder_mpd, &reorderPath[i].buf);

                    reorderPath[i].reorder_mem.Set(reorderPath[i].reorder_mpd, reorderPath[i].buf);
                    CK(mkldnn_reorder_primitive_desc_create(&reorderPath[i].reorder_pd, reorderPath[i].input_mpd, reorderPath[i].reorder_mpd));
                    mkldnn_primitive_at_t inputs[] = {mkldnn_primitive_at(reorderPath[i].input_mem, 0)};
                    const_mkldnn_primitive_t outputs[] = {reorderPath[i].reorder_mem};
                    CK(mkldnn_primitive_create(&reorderPath[i].reorder, reorderPath[i].reorder_pd, inputs, outputs));
                    //mkldnn_primitive_desc_query_memory_d(reorderPath[i].reorder.get_mpd_at(mkldnn_query_dst_pd));
                }
                if (i == 0) {
                    MemCopy(output_dims, reorderPath[i].input_md->dims, output_rank);
                    output_dims[mkldnn_concat_dim] = 0; // we'll recollect concatenated dim size in the loop that follows
                }
                for (int d = 0; d < output_rank; ++d) {
                    if (d == mkldnn_concat_dim) {
                        output_dims[d] += reorderPath[i].input_md->dims[d];
                    } else {
                        OP_REQUIRES(context, reorderPath[i].input_md->dims[d] == output_dims[d],
                            errors::InvalidArgument(type_string(), ": Dimension mismatch: ",
                                " expected ", output_dims[d], ", got ", reorderPath[i].input_md->dims[d],
                                " for ", (reorderPath[i].reorder ? "reordered" : ""), " input ", i, " dim ", d, " to operation ", name()));
                    }
                }
            }
        }

        // Describe output memory layout
        Mkldnn_md output_md;
        output_md.Set(T(), output_rank, output_dims, preferred_memory_format);

        // Prepare input and output primitive descriptors
        std::vector<const_mkldnn_primitive_desc_t> input_pds(numInputs);
        for (size_t i = 0; i < numInputs; ++i) {
            auto* input_ppd = input_shapes[i].get_mpd();
            if (reorderPath[i].reorder) {
                input_pds[i] = reorderPath[i].reorder_mpd;
            } else {
                input_pds[i] = reorderPath[i].input_mpd;
            }
        }

        // Create the concat primitive descriptor
        Mkldnn_pd concat_pd;
        CK(mkldnn_concat_primitive_desc_create(&concat_pd, &output_md,
            static_cast<int>(input_pds.size()), mkldnn_concat_dim, input_pds.data()));

        // Allocate output tensor
        MkldnnShape output_shape_mkl;
        output_shape_mkl.SetMkldnnTensor(true);
        output_shape_mkl.SetMkldnnLayout(concat_pd.get_mpd_at(mkldnn_query_dst_pd));

        Mkldnn_md tf_output_md;
        tf_output_md.Set(T(), output_rank, output_dims, data_format_ == FORMAT_NHWC ? mkldnn_nhwc : mkldnn_nchw);
        Mkldnn_mpd tf_output_mpd(&tf_output_md);
        output_shape_mkl.SetTfLayout(tf_output_mpd);

        TensorShape output_shape_tf;
        output_shape_tf.AddDim(output_shape_mkl.GetMemorySize() / sizeof(T));

        Tensor *output_tensor = nullptr;
        AllocateOutputSetMkldnnShape(context, 0, &output_tensor, output_shape_tf, output_shape_mkl);

        // Describe the input/output memory primitives for concat
        std::vector<mkldnn_primitive_at_t> inputs;
        for (int i = 0; i < numInputs; ++i) {
            if (reorderPath[i].reorder) {
                inputs.push_back(mkldnn_primitive_at(reorderPath[i].reorder_mem, 0));
                CHECK(inputs.back().primitive) << " from reorder for i=" << i;
            } else {
                inputs.push_back(mkldnn_primitive_at(reorderPath[i].input_mem, 0));
                CHECK(inputs.back().primitive) << " from input for i=" << i;
            }
        }

        // Describe the outputs of concat
        Mkldnn_mpd output_mpd(&output_md);
        Mkldnn_memory output_mem(output_mpd, output_tensor->flat<T>().data());
        const_mkldnn_primitive_t outputs[] = {output_mem};

        VLOG(1) << "MKLDNN: " << type_string() << " '" << name() << "'"
            //<< " in[0]=" << ToDebugString(input_mem[0]->get_mpd())
            //<< (input_mem.size() > 1 ? (string(" in[1]=") + ToDebugString(input_mem[1]->get_mpd())) : "")
            //<< " out=" << ToDebugString(output_mem.get_mpd())
            ;

        // Create the concat primitive
        Mkldnn_primitive concat;
        CK(mkldnn_primitive_create(&concat, concat_pd, inputs.data(), outputs));

        // Execute optional reorder of inputs and the concat
        std::vector<mkldnn_primitive_t> net;
        for (int i = 0; i < numInputs; ++i) {
            if (reorderPath[i].reorder) {
                net.push_back(reorderPath[i].reorder);
            }
        }
        net.push_back(concat);

        Mkldnn_stream stream;
        CK(mkldnn_stream_submit(stream, net.size(), net.data(), nullptr));
        CK(mkldnn_stream_wait(stream, net.size(), nullptr));
    }


private:
    TensorFormat data_format_;
};

/* Use optimized concat for float type only */
#define REGISTER_MKL_CPU(type)                                              \
    REGISTER_KERNEL_BUILDER(Name("_MkldnnConcat")                           \
        .Device(DEVICE_CPU)                                                 \
        .TypeConstraint<type>("T")                                          \
        .HostMemory("concat_dim")                                           \
        .Label(mkl_op_registry::kMkldnnOpLabel),                            \
        MkldnnConcatOp<CPUDevice, type, NAME_IS_CONCAT_DIM>)                \
    REGISTER_KERNEL_BUILDER(Name("_MkldnnConcatV2")                         \
        .Device(DEVICE_CPU)                                                 \
        .TypeConstraint<type>("T")                                          \
        .TypeConstraint<int32>("Tidx")                                      \
        .HostMemory("axis")                                                 \
        .Label(mkl_op_registry::kMkldnnOpLabel),                            \
        MkldnnConcatOp<CPUDevice, type, NAME_IS_AXIS>)

TF_CALL_float(REGISTER_MKL_CPU);

}  // namespace tensorflow

#endif /* ARCADIA_BUILD_ROOT */

