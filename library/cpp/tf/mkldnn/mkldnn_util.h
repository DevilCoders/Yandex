#pragma once

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

#include <algorithm>
#include <iomanip>
#include <string>
#include <vector>

#define EIGEN_USE_THREADS

#include "tensorflow/core/framework/common_shape_fns.h"
#include "tensorflow/core/framework/numeric_op.h"
#include "tensorflow/core/framework/op.h"
#include "tensorflow/core/framework/op_kernel.h"
#include "tensorflow/core/framework/register_types.h"
#include "tensorflow/core/framework/tensor.h"
#include "tensorflow/core/framework/tensor_shape.h"
#include "tensorflow/core/framework/tensor_slice.h"
#include "tensorflow/core/kernels/bounds_check.h"
#include "tensorflow/core/lib/core/errors.h"
#include "tensorflow/core/lib/gtl/array_slice.h"
#include "tensorflow/core/lib/strings/numbers.h"
#include "tensorflow/core/lib/strings/str_util.h"
#include "tensorflow/core/platform/logging.h"
#include "tensorflow/core/platform/macros.h"
#include "tensorflow/core/util/padding.h"
#include "tensorflow/core/util/port.h"
#include "tensorflow/core/util/tensor_format.h"

#include "contrib/deprecated/onednn/include/mkldnn.h"
using string = TString;

// The file contains a number of utility classes and functions used by MKL
// enabled kernels

namespace tensorflow {
#define CK(expr)                                 \
    do {                                         \
        mkldnn_status_t s(expr);                 \
        if (s != mkldnn_success)                 \
            Y_FAIL("%s returns %i\n", #expr, s); \
    } while (false)

    inline mkldnn_memory_format_t MkldnnMemoryFormatFor(const TensorFormat& tensorFormat) {
        if (tensorFormat == FORMAT_NHWC)
            return mkldnn_nhwc;
        if (tensorFormat == FORMAT_NCHW)
            return mkldnn_nchw;
        return mkldnn_any;
    }

    template <typename T>
    mkldnn_data_type_t MkldnnDataTypeFor();
    template <>
    inline mkldnn_data_type_t MkldnnDataTypeFor<float>() {
        return mkldnn_f32;
    }

    struct Mkldnn {
        static mkldnn_engine_t the_engine; // defined in mkldnn_conv_ops.cc
        static mkldnn_engine_t engine() {
            return the_engine;
        }
    };

    struct Mkldnn_md: public mkldnn_memory_desc_t { // memory descriptor
        Mkldnn_md() {
        }
        template <typename T, typename... Args>
        Mkldnn_md(T&& t, Args&&... args) {
            Set(t, args...);
        }
        template <typename T>
        void Set(const T&, int rank, const mkldnn_dims_t dims, mkldnn_memory_format_t format) {
            CK(mkldnn_memory_desc_init(this, rank, dims, MkldnnDataTypeFor<T>(), format));
        }
        template <typename T>
        void Set(const T& t, int rank, std::initializer_list<int> ildims, mkldnn_memory_format_t format) {
            Set(t, rank, ildims.begin(), format);
        }
        template <typename... Void>
        inline size_t _blk_off() const {
            return layout_desc.blocking.offset_padding;
        }
        template <typename T, typename... Args>
        inline size_t _blk_off(Args... args, T xn) const {
            return size_t(xn) * layout_desc.blocking.strides[0][sizeof...(args)] + _blk_off<Args...>(args...);
        }
        template <typename... Args>
        inline size_t blk_off(Args... args) const {
            return _blk_off<Args...>(args...);
        }
    };

    struct Mkldnn_mpd;

    struct Mkldnn_pd { // primitive desc
        mkldnn_primitive_desc_t pd;
        Mkldnn_pd()
            : pd(0)
        {
        }
        Mkldnn_pd(const Mkldnn_pd&) = delete;
        ~Mkldnn_pd() {
            CK(mkldnn_primitive_desc_destroy(pd));
            pd = nullptr;
        }
        operator const_mkldnn_primitive_desc_t() const {
            return pd;
        }
        const_mkldnn_primitive_desc_t get_mpd_at(mkldnn_query_t what, int idx = 0) const {
            return mkldnn_primitive_desc_query_pd(pd, what, idx);
        }
        mkldnn_primitive_desc_t* operator&() {
            return &pd;
        }
    };

    static inline const Mkldnn_md* GetMkldnnMd(const_mkldnn_primitive_desc_t pd) {
        return (Mkldnn_md*)mkldnn_primitive_desc_query_memory_d(pd);
    }

    struct Mkldnn_mpd: public Mkldnn_pd { // memory primitive desc
        const mkldnn_memory_desc_t* mymd;
        Mkldnn_mpd() {
        }
        Mkldnn_mpd(const Mkldnn_mpd&) = delete;
        template <typename... Args>
        Mkldnn_mpd(Args&&... args) {
            Set(args...);
        }
        void Set(const mkldnn_memory_desc_t* md) {
            CHECK(!pd);
            CK(mkldnn_memory_primitive_desc_create(&pd, md, Mkldnn::engine()));
            mymd = GetMkldnnMd(pd);
        }
        void Set(const_mkldnn_primitive_desc_t mpd) {
            CHECK(!pd);
            CK(mkldnn_primitive_desc_clone(&pd, mpd));
            mymd = GetMkldnnMd(pd);
        }
        void SerializeTo(uint8* buf) const {
            memcpy(buf, GetMkldnnMd(*this), sizeof(mkldnn_memory_desc_t));
        }
        void DeserializeFrom(const uint8* buf) {
            mkldnn_memory_desc_t md;
            memcpy(&md, buf, sizeof(mkldnn_memory_desc_t));
            Set(&md);
        }
        size_t SerializationSize() const {
            return sizeof(mkldnn_memory_desc_t);
        }
        size_t GetMemorySize() const {
            return mkldnn_memory_primitive_desc_get_size(pd);
        }
        int GetSize(char c) const {
            auto md = GetMkldnnMd(*this);
            switch (md->format) {
                case mkldnn_nchw:
                case mkldnn_nhwc:
                case mkldnn_nChw8c:
                    switch (c) {
                        case 'n':
                        case 'N':
                            return md->dims[0];
                        case 'c':
                        case 'C':
                            return md->dims[1];
                        case 'h':
                        case 'H':
                            return md->dims[2];
                        case 'w':
                        case 'W':
                            return md->dims[3];
                    }
                    break;
                case mkldnn_oihw:
                case mkldnn_Ohwi8o:
                    switch (c) {
                        case 'o':
                        case 'O':
                            return md->dims[0];
                        case 'i':
                        case 'I':
                            return md->dims[1];
                        case 'h':
                        case 'H':
                            return md->dims[2];
                        case 'w':
                        case 'W':
                            return md->dims[3];
                    }
                    break;
                case mkldnn_goihw:
                    switch (c) {
                        case 'g':
                        case 'G':
                            return md->dims[0];
                        case 'o':
                        case 'O':
                            return md->dims[1];
                        case 'i':
                        case 'I':
                            return md->dims[2];
                        case 'h':
                        case 'H':
                            return md->dims[3];
                        case 'w':
                        case 'W':
                            return md->dims[4];
                    }
                    break;
                case mkldnn_nc:
                    switch (c) {
                        case 'n':
                        case 'N':
                            return md->dims[0];
                        case 'c':
                        case 'C':
                            return md->dims[1];
                    }
                    break;
                default:
                    break;
            }
            CHECK(false) << "Unknown mkldnn_memory_format_t " << md->format << " or key " << c;
        }
    };

    inline bool equal(const_mkldnn_primitive_desc_t a, const_mkldnn_primitive_desc_t b) {
        return mkldnn_memory_primitive_desc_equal(a, b);
    }

    struct Mkldnn_primitive { // operation|memory primitive
        mkldnn_primitive_t prim;
        Mkldnn_primitive()
            : prim(0)
        {
        }
        Mkldnn_primitive(const Mkldnn_primitive&) = delete;
        ~Mkldnn_primitive() {
            CK(mkldnn_primitive_destroy(prim));
            prim = nullptr;
        }
        mkldnn_primitive_t* operator&() {
            return &prim;
        }
        operator mkldnn_primitive_t() {
            return prim;
        }
    };

    struct Mkldnn_memory: public Mkldnn_primitive {
        Mkldnn_memory() {
        }
        template <typename... Args>
        Mkldnn_memory(Args&&... args) {
            Set(args...);
        }
        template <typename D>
        void Set(const_mkldnn_primitive_desc_t mpd, D* buf = nullptr) {
            CK(mkldnn_primitive_create(&prim, mpd, nullptr, nullptr));
            if (buf)
                CK(mkldnn_memory_set_data_handle(prim, (void*)buf));
        }
        const_mkldnn_primitive_desc_t get_mpd() const {
            const_mkldnn_primitive_desc_t res;
            CK(mkldnn_primitive_get_primitive_desc(prim, &res));
            return res;
        }
        void* get_data_handle() const {
            void* res;
            CK(mkldnn_memory_get_data_handle(prim, &res));
            return res;
        }
    };

    struct Mkldnn_stream {
        mkldnn_stream_t stream;
        Mkldnn_stream() {
            CK(mkldnn_stream_create(&stream, mkldnn_eager));
        }
        ~Mkldnn_stream() {
            CK(mkldnn_stream_destroy(stream));
        }
        operator mkldnn_stream_t() {
            return stream;
        }
    };

    typedef enum { W = 0,
                   H = 1,
                   C = 2,
                   N = 3 } MkldnnDims;

    // This class encapsulates all the meta data that is associated with an MKL
    // tensor. A tensor is an MKL tensor if it was created as the result of an
    // MKL operation, and did not go through a conversion to a standard
    // Tensorflow tensor.

    class MkldnnShape {
    public:
        MkldnnShape() {
        }
        TF_DISALLOW_COPY_AND_ASSIGN(MkldnnShape); // Cannot copy

        ~MkldnnShape() {
        }

        const bool IsMkldnnTensor() const {
            return isMkldnnTensor_;
        }

        void SetMkldnnTensor(const bool isMkldnnTensor) {
            isMkldnnTensor_ = isMkldnnTensor;
        }

        void SetMkldnnLayout(const Mkldnn_mpd& mpd) {
            mklLayout_.Set(mpd);
        }

        void SetTfLayout(const Mkldnn_mpd& mpd) {
            tfLayout_.Set(mpd);
        }

        const_mkldnn_primitive_desc_t get_mpd() const {
            return isMkldnnTensor_ ? mklLayout_.pd
                                   : tfLayout_.pd;
        }

        size_t GetMemorySize() const {
            return isMkldnnTensor_ ? mklLayout_.GetMemorySize()
                                   : tfLayout_.GetMemorySize();
        }

        void get_isMkldnnTensor(const uint8* buf) {
            isMkldnnTensor_ = bool(buf[0]);
        }
        void put_isMkldnnTensor(uint8* buf) const {
            buf[0] = (uint8)isMkldnnTensor_;
        }

        size_t SerializationSize() const {
            return 1 + mklLayout_.SerializationSize() + tfLayout_.SerializationSize();
        }

        void DeSerializeMkldnnShape(const uint8* buf, size_t buf_size) {
            // Make sure buffer holds at least isMkldnnTensor_
            CHECK(buf_size >= sizeof(size_t)) << "Bufsize too small in DeSerialize";
            get_isMkldnnTensor(buf);
            if (isMkldnnTensor_) { // If it is an MKL Tensor then read the rest
                mklLayout_.DeserializeFrom(&buf[1]);
                tfLayout_.DeserializeFrom(&buf[1 + mklLayout_.SerializationSize()]);
            }
        }

        void SerializeMkldnnShape(uint8* buf, size_t buf_size) const {
            CHECK(buf_size >= this->SerializationSize()) << "Bufsize too small to Serialize";
            put_isMkldnnTensor(buf);
            if (isMkldnnTensor_) {
                mklLayout_.SerializeTo(&buf[1]);
                tfLayout_.SerializeTo(&buf[1 + mklLayout_.SerializationSize()]);
            }
        }

        int64 GetSize(char c) const {
            return isMkldnnTensor_ ? mklLayout_.GetSize(c) : tfLayout_.GetSize(c);
        }

        const Mkldnn_mpd& mkl_mpd() const {
            return mklLayout_;
        }
        const Mkldnn_mpd& tf_mpd() const {
            return tfLayout_;
        }

    private:
        bool isMkldnnTensor_ = false; // Flag to indicate if the tensor is an MKL tensor
        Mkldnn_mpd mklLayout_;        // Description of MKL layout
        Mkldnn_mpd tfLayout_;         // Description of TF layout
    };

    // List of MkldnnShape objects. Used in Concat/Split layers.
    typedef std::vector<MkldnnShape> MkldnnShapeList;

    // Check if all tensors specified by MkldnnShapes are MKL tensors.
    inline bool AreAllMkldnnTensors(const MkldnnShapeList& shapes) {
        for (auto& s : shapes) {
            if (!s.IsMkldnnTensor()) {
                return false;
            }
        }
        return true;
    }

    // Since our ops are going to produce and also consume N addition tensors
    // (Mkldnn) for N Tensorflow tensors, we can have following different
    // orderings among these 2N tensors.
    //
    // E.g., for Tensorflow tensors A, B, and C, our ops will produce and
    // consume A_m, B_m, and C_m additionally.
    //
    // INTERLEAVED: in this case 2N tensors are interleaved. So for above
    //              example, the ordering looks like: A, A_m, B, B_m, C, C_m.
    //
    // CONTIGUOUS: in thi case N Tensorflow tensors are contiguous followed
    //             by N Mkldnn tensors. So for above example, the ordering looks
    //             like: A, B, C, A_m, B_m, C_m
    //
    // Following APIs map index of original Tensorflow tensors to their appropriate
    // position based on selected ordering. For contiguous ordering, we need to know
    // the total number of tensors (parameter total).
    //
    typedef enum { TENSORS_INTERLEAVED,
                   TENSORS_CONTIGUOUS } MkldnnTfTensorOrdering;
    // NOTE: Currently, we use contiguous ordering. If you change this, then you
    // would need to change Mkldnn op definitions in nn_ops.cc.
    constexpr MkldnnTfTensorOrdering kTensorOrdering = TENSORS_CONTIGUOUS;

    // Get index of MetaData tensor from index 'n' of Data tensor.
    inline int DataIndexToMetaDataIndex(int n, int total_tensors) {
        if (kTensorOrdering == MkldnnTfTensorOrdering::TENSORS_INTERLEAVED) {
            // For interleaved ordering, Mkldnn tensor follows immediately after
            // Tensorflow tensor.
            return n + 1;
        } else {
            CHECK_EQ(kTensorOrdering, MkldnnTfTensorOrdering::TENSORS_CONTIGUOUS);
            // For contiguous ordering, Mkldnn tensor is n+total_tensors / 2 away.
            return n + total_tensors / 2;
        }
    }

    int inline GetTensorDataIndex(int n, int total_tensors) {
        if (kTensorOrdering == MkldnnTfTensorOrdering::TENSORS_INTERLEAVED) {
            return 2 * n; // index corresponding to nth input/output tensor
        } else {
            CHECK_EQ(kTensorOrdering, MkldnnTfTensorOrdering::TENSORS_CONTIGUOUS);
            return n;
        }
    }

    int inline GetTensorMetaDataIndex(int n, int total_tensors) {
        // Get index for TensorData first and then use mapping function
        // to get TensorMetaData index from TensorData index.
        int tidx = GetTensorDataIndex(n, total_tensors);
        return DataIndexToMetaDataIndex(tidx, total_tensors);
    }

    // Gets the actual input
    inline const Tensor& MkldnnGetInput(OpKernelContext* ctext, int n) {
        return ctext->input(GetTensorDataIndex(n, ctext->num_inputs()));
    }

    inline void perm(int rank, mkldnn_dims_t& res, const TensorShape& shape, std::initializer_list<int> p) {
        if (!p.size()) {
            for (int i = 0; i < rank; ++i) {
                res[i] = static_cast<int>(shape.dim_size(i));
            }
        } else {
            CHECK(rank == p.size());
            const int* q = p.begin();
            for (int i = 0; i < p.size(); ++i) {
                res[i] = static_cast<int>(shape.dim_size(q[i]));
            }
        }
    }

    namespace {
        // Current Arcadia MKLDNN does not define mkldnn_hwio
        class MkldnnMemoryFormat {
            int f;

        public:
            enum Extra : int {
                mkldnn_ghwio = -1,
                mkldnn_hwio = -2,
            };
            MkldnnMemoryFormat(mkldnn_memory_format_t x)
                : f(x)
            {
            }
            MkldnnMemoryFormat& operator=(const MkldnnMemoryFormat&) = default;
            MkldnnMemoryFormat(enum Extra x)
                : f(x)
            {
            }
            operator int() const {
                return f;
            }
        };
    }

    Mkldnn_md GuessTfLayout(
        OpKernelContext* ctext,
        int n,
        MkldnnMemoryFormat format) {

        const Tensor& tensor = MkldnnGetInput(ctext, n);
        const TensorShape& shape = tensor.shape();
        int rank = shape.dims();
        mkldnn_dims_t dims;
        switch (format) {
            case mkldnn_nhwc:
                CHECK_EQ(rank, 4);
                perm(rank, dims, shape, {0, 3, 1, 2});
                format = mkldnn_nhwc;
                break;
            case MkldnnMemoryFormat::mkldnn_ghwio:
                CHECK_EQ(rank, 5);
                perm(rank, dims, shape, {0, 4, 3, 1, 2});
                format = mkldnn_goihw;
                break;
            case MkldnnMemoryFormat::mkldnn_hwio:
                CHECK_EQ(rank, 4);
                perm(rank, dims, shape, {3, 2, 0, 1});
                format = mkldnn_oihw;
                break;
            case mkldnn_any:
            case mkldnn_nchw:
            case mkldnn_x:
                perm(rank, dims, shape, {});
                if (format == mkldnn_any) {
                    // Pick a good format for this rank
                    if (rank == 4) {
                        format = mkldnn_nchw;
                    } else if (rank < 2) {
                        format = mkldnn_x;
                    } else if (rank == 2) {
                        format = mkldnn_nc;
                    } else {
                        CHECK(false) << "Cannot pick a good mkldnn_format for rank " << rank;
                    }
                }
                break;
            default:
                CHECK(false) << "Unknown MkldnnMemoryFormat " << int(format);
                break;
        }
        Mkldnn_md md(float(), rank, dims, static_cast< ::mkldnn_memory_format_t>(int(format)));
        if (mkldnn_strides_t& strides = md.layout_desc.blocking.strides[0]) {
            // Fix strides for formats not supported by mkldnn
            if (md.format == mkldnn_oihw) {
                int o = md.dims[0], i = md.dims[1], h = md.dims[2], w = md.dims[3];
                strides[0] = 1;
                strides[1] = o;
                strides[3] = o * i;
                strides[2] = o * i * w;
                (void)h;
            } else if (md.format == mkldnn_goihw) {
                int g = md.dims[0], o = md.dims[1], i = md.dims[2], h = md.dims[3], w = md.dims[4];
                strides[1] = 1;
                strides[2] = o;
                strides[4] = o * i;
                strides[3] = o * i * w;
                strides[0] = o * i * w * h;
                (void)g;
            }
        }
        return md;
    }

    // Get the MKL shape from the second string tensor
    inline void GetMkldnnShape(OpKernelContext* ctext, int n, MkldnnShape* mklshape,
                               MkldnnMemoryFormat format) {
        const int metaDataIdx = GetTensorMetaDataIndex(n, ctext->num_inputs());
        mklshape->DeSerializeMkldnnShape(
            ctext->input(metaDataIdx).flat<uint8>().data(),
            ctext->input(metaDataIdx).flat<uint8>().size() * sizeof(uint8));
        if (!mklshape->IsMkldnnTensor()) {
            Mkldnn_md md = GuessTfLayout(ctext, n, format);
            Mkldnn_mpd mpd(&md);
            mklshape->SetTfLayout(mpd);
        }
    }

    inline void GetMkldnnInputList(OpKernelContext* ctext, StringPiece name,
                                   OpInputList* input_tensors) {
        CHECK_NOTNULL(input_tensors);
        (void)ctext->input_list(name, input_tensors);
    }

    inline void GetMkldnnShapeList(OpKernelContext* ctext, StringPiece name,
                                   MkldnnShapeList* mkl_shapes) {
        OpInputList input_mkl_tensors;
        GetMkldnnInputList(ctext, strings::StrCat("mkl_", name), &input_mkl_tensors);

        for (int i = 0; i < input_mkl_tensors.size(); i++) {
            (*mkl_shapes)[i].DeSerializeMkldnnShape(
                input_mkl_tensors[i].flat<uint8>().data(),
                input_mkl_tensors[i].flat<uint8>().size() * sizeof(uint8));
        }
    }

    // Allocate the second output tensor that will contain
    // the MKL shape serialized
    inline void AllocateOutputSetMkldnnShape(OpKernelContext* ctext, int n,
                                             const MkldnnShape& mkl_shape) {
        Tensor* second_tensor = nullptr;
        TensorShape second_shape;
        second_shape.AddDim(mkl_shape.SerializationSize());
        const int metaDataIdx = GetTensorMetaDataIndex(n, ctext->num_outputs());
        OP_REQUIRES_OK(ctext, ctext->allocate_output(
                                  metaDataIdx,
                                  second_shape, &second_tensor));
        mkl_shape.SerializeMkldnnShape(
            second_tensor->flat<uint8>().data(),
            second_tensor->flat<uint8>().size() * sizeof(uint8));
    }

    // Allocate the output tensor, create a second output tensor that will contain
    // the MKL shape serialized
    inline void AllocateOutputSetMkldnnShape(OpKernelContext* ctext, int n,
                                             Tensor** output,
                                             const TensorShape& tf_shape,
                                             const MkldnnShape& mkl_shape) {
        Tensor* second_tensor = nullptr;
        TensorShape second_shape;
        second_shape.AddDim(mkl_shape.SerializationSize());
        const int dataIdx = GetTensorDataIndex(n, ctext->num_outputs());
        const int metaDataIdx = GetTensorMetaDataIndex(n, ctext->num_outputs());
        OP_REQUIRES_OK(
            ctext, ctext->allocate_output(dataIdx, tf_shape, output));
        OP_REQUIRES_OK(ctext, ctext->allocate_output(
                                  metaDataIdx,
                                  second_shape, &second_tensor));
        mkl_shape.SerializeMkldnnShape(
            second_tensor->flat<uint8>().data(),
            second_tensor->flat<uint8>().size() * sizeof(uint8));
    }

    // Allocates a temp tensor and returns the data buffer for temporary storage.
    // Currently
    // we only support F32, will need to templatize if other types are added
    inline void AllocTmpBuffer(OpKernelContext* context, Tensor* tensor_out,
                               const Mkldnn_mpd& mpd, void** buf_out) {
        TensorShape tf_shape;
        tf_shape.AddDim(mpd.GetMemorySize() / sizeof(float) + 1);
        OP_REQUIRES_OK(context, context->allocate_temp(DataTypeToEnum<float>::v(),
                                                       tf_shape, tensor_out));
        *buf_out = static_cast<void*>(tensor_out->flat<float>().data());
    }

    inline int64 GetMkldnnTensorDim(const MkldnnShape& mkl_shape, char dimension) {
        return mkl_shape.GetSize(dimension);
    }

    inline Status GetWindowedOutputSizeVerbose_int(int input_size, int filter_size,
                                                   int stride, Padding padding_type, int* out_size, int* padding, int* padding_after) {
        int64 out_size_raw, padding_raw, padding_after_raw;
        Status s(GetWindowedOutputSizeVerbose(input_size, filter_size, stride, padding_type,
                                              &out_size_raw, &padding_raw, &padding_after_raw));
        *out_size = static_cast<int>(out_size_raw);
        *padding = static_cast<int>(padding_raw);
        *padding_after = static_cast<int>(padding_after_raw);
        return s;
    }

    namespace mkl_op_registry {
        static const char* kMkldnnOpLabel = "MkldnnOp";
        static const char* kMkldnnOpLabelPattern = "label='MkldnnOp'";

        // Check whether opname with type T is registered as MKL-compliant.
        //
        // @input: name of the op
        // @input: T datatype to be used for checking op
        // @return: true if opname is registered as Mkldnn op
        static inline bool IsMkldnnOp(StringPiece op_name, DataType T) {
            string kernel = KernelsRegisteredForOp(op_name);
            bool result =
                kernel.find(kMkldnnOpLabelPattern) != string::npos && (T == DT_FLOAT);
            if (result) {
                VLOG(1) << "mkl_op_registry::" << op_name << " is " << kMkldnnOpLabel;
            }
            return result;
        }

    }
}
#endif /* ARCADIA_BUILD_ROOT */
