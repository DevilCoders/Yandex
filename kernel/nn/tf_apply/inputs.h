#pragma once

#include <kernel/nn/tf_apply/utils.h>
#include <util/generic/array_ref.h>
#include <util/generic/bt_exception.h>
#include <util/generic/deque.h>
#include <util/generic/maybe.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <util/generic/yexception.h>

#include <tensorflow/core/framework/types.h>
#include <tensorflow/core/graph/default_device.h>
#include <tensorflow/core/public/session.h>


namespace NTFModel {
    using std::pair;
    using TTFInt = long long;
    using TTFTensorShape = tensorflow::TensorShape;
    using TTFTensor = tensorflow::Tensor;
    using ETFDataType = tensorflow::DataType;

    class TTFException : public yexception {};

    template <ETFDataType DataType>
    using TCppTypeByDataType = typename tensorflow::EnumToDataType<DataType>::Type;

    template <typename T>
    inline constexpr ETFDataType DataTypeByCppType() {
        return tensorflow::DataTypeToEnum<T>::value;
    }

    enum class ETFInputType {
        Dense1D,
        Sparse1D
    };

    template <typename TDataType>
    struct TTFDense1DInputType {
        static constexpr ETFInputType InputType = ETFInputType::Dense1D;
        static constexpr ETFDataType TFDataType = DataTypeByCppType<TDataType>();
    };

    template <typename TDataType>
    struct TTFSparseInputType {
        static constexpr ETFInputType InputType = ETFInputType::Sparse1D;
        static constexpr ETFDataType TFDataType = DataTypeByCppType<TDataType>();

        struct TSparseRow { // (index_in_row, value)
            TVector<TTFInt> ColumnIndices;
            TVector<TDataType> Data;
        };
    };

    template <typename... TInTypes>
    struct TTFInputSignature;

    template <typename TSigType, size_t Index>
    struct TTFInputTypeByIndex;

    template <typename TInTypeX, typename... TInTypes>
    struct TTFInputTypeByIndex<TTFInputSignature<TInTypeX, TInTypes...>, 0> {
        using TResult = TInTypeX;
    };

    template <typename TInTypeX, typename... TInTypes, size_t Index>
    struct TTFInputTypeByIndex<TTFInputSignature<TInTypeX, TInTypes...>, Index> {
        static_assert(Index > 0, "");
        using TResult = typename TTFInputTypeByIndex<TTFInputSignature<TInTypes...>, Index - 1>::TResult;
    };

    template <>
    struct TTFInputSignature<> {
        static constexpr size_t Size = 0;
    };

    template <typename TInTypeX, typename... TInTypes>
    struct TTFInputSignature<TInTypeX, TInTypes...> {
        static constexpr size_t Size = 1 + TTFInputSignature<TInTypes...>::Size;

        template <size_t Index>
        struct TByIndex {
            using TResult = typename TTFInputTypeByIndex<TTFInputSignature<TInTypeX, TInTypes...>, Index>::TResult;
        };
    };

    struct TBatch {
        TTFInt NumSamples = 0;  // numbers of samples in batch
        TVector<TTFTensor> Tensors;
        // size of vector equals number of model inputs (with splitting ,
        // e.g. Sparse input = 3 tensors)

        TBatch(
            TTFInt numSamples,
            TVector<TTFTensor> tensors)
            : NumSamples(numSamples)
            , Tensors(std::move(tensors))
        {
        }
    };

    class ITFInput {
    public:
        virtual ~ITFInput() = default;
        virtual TTFTensorShape GetSampleShape() const = 0;
        virtual TTFInt GetNumSamples() const = 0;
        virtual TTFInt GetNumBatches() const = 0;
        virtual TVector<TString> GetInputNames(TStringBuf path) const = 0;
        virtual TBatch GetBatch(size_t index) const = 0;
    };

    template <typename TDataType>
    class TTFDense1DInput: public ITFInput {
    public:
        TTFDense1DInput(
            TTFInt sampleSize)
            : SampleSize(sampleSize)
        {
        }

        TTFDense1DInput(
            TArrayRef<const TDataType> data)
            : SampleSize(data.size())
        {
            Append(data);
        }

        TTFDense1DInput(
            const TVector<TVector<TDataType>>& data,
            TTFInt batchSize)
        {
            Y_ENSURE_EX(
                !data.empty(),
                TTFException() << "empty data given"
            );
            SampleSize = data[0].size();
            Append(data, batchSize);
        }

        TTFDense1DInput(
            const TVector<TArrayRef<const TDataType>>& data,
            TTFInt batchSize)
        {
            Y_ENSURE_EX(
                !data.empty(),
                TTFException() << "empty data given"
            );
            SampleSize = data[0].size();
            Append(data, batchSize);
        }

        void Reset() {
            Batches.clear();
            NumSamples = 0;
        }

        TTFTensorShape GetSampleShape() const override {
            return TTFTensorShape{SampleSize,};
        }

        TTFInt GetNumSamples() const override {
            return NumSamples;
        }

        TTFInt GetNumBatches() const override {
            return Batches.size();
        }

        TVector<TString> GetInputNames(TStringBuf path) const override {
            return TVector<TString>{TString{path},};
        }

        TBatch GetBatch(size_t index) const override {
            Y_ENSURE_EX(index < Batches.size(),
                TTFException()
                    << "batch index " << index
                    << " out of range [0;" << Batches.size() << ")");
            return Batches[index];
        };

        void Append(TArrayRef<const TDataType> item) {
            AppendBatch(&item, &item + 1);
        };

        void Append(const TVector<TArrayRef<const TDataType>>& items, TTFInt batchSize) {
            Append(items.begin(), items.end(), batchSize);
        }

        void Append(const TVector<TVector<TDataType>>& items, TTFInt batchSize) {
            Append(items.begin(), items.end(), batchSize);
        }

        template <class TIter>
        void Append(TIter begin, TIter end, TTFInt batchSize) {
            Y_ENSURE_EX(begin < end,
                TTFException() << "empty sequence given"
            );

            if (batchSize == 0) {
                AppendBatch(begin, end);
            } else {
                TTFInt samples = TTFInt(end - begin);
                TTFInt numBatches = samples / batchSize;
                for (; numBatches; --numBatches, begin += batchSize) {
                    AppendBatch(begin, begin + batchSize);
                }
                if (begin < end) {
                    AppendBatch(begin, end);
                }
            }
        }

        void AppendBatch(const TVector<TVector<TDataType>>& items) {
            return AppendBatch(items.begin(), items.end());
        }

        template<typename TIter>
        void AppendBatch(TIter begin, TIter end) {
            Y_ENSURE_EX(begin < end,
                TTFException() << "empty sequence given"
            );
            TTFInt samples = TTFInt(end - begin);
            TTFTensor newTensor(
                DataTypeByCppType<TDataType>(),
                TTFTensorShape{samples, SampleSize});
            TDataType* data = newTensor.flat<TDataType>().data();
            auto it = begin;
            for (size_t i = 0; it < end; ++i, ++it) {
                Y_ENSURE_EX(
                    it->size() == size_t(SampleSize),
                    TTFException()
                        << "expected dim: " << SampleSize
                        << ", but given: " << it->size()
                );
                MemCopy(data + i * SampleSize, (*it).data(), SampleSize);
            }
            Batches.emplace_back(samples, WrapInVector(std::move(newTensor)));
            NumSamples += samples;
        }

    private:
        TTFInt SampleSize = 0;
        TTFInt NumSamples = 0;

        TVector<TBatch> Batches;
    };

    template <typename TDataType>
    class TTFSparse1DInput
       : public ITFInput {
    public:
        using TSparseRow = typename TTFSparseInputType<TDataType>::TSparseRow;

        TTFSparse1DInput(
            TTFInt sampleSize)
            : SampleSize(sampleSize)
        {
        }

        TTFSparse1DInput(
            TTFInt sampleSize,
            const TSparseRow& data)
            : SampleSize(sampleSize)
        {
            Append(data);
        }

        TTFSparse1DInput(
            TTFInt sampleSize,
            TArrayRef<const TSparseRow> data,
            TTFInt batchSize)
            : SampleSize(sampleSize)
        {
            Append(data, batchSize);
        }

        void Reset() {
            Batches.clear();
            NumSamples = 0;
        }

        TTFTensorShape GetSampleShape() const override {
            return TTFTensorShape{SampleSize,};
        }
        TTFInt GetNumSamples() const override {
            return NumSamples;
        }
        TTFInt GetNumBatches() const override {
            return Batches.size();
        }
        TVector<TString> GetInputNames(TStringBuf path) const override {
            return TVector<TString>{
                    TString{path} + "/indices",
                    TString{path} + "/values",
                    TString{path} + "/shape"
            };
        }

        TBatch GetBatch(size_t index) const override {
            Y_ENSURE_EX(index < Batches.size(),
                TTFException()
                    << "batch index " << index
                    << " out of range [0;" << Batches.size() << ")");
            return Batches[index];
        };

        void Append(const TSparseRow& item) {
            AppendBatch(&item, &item + 1);
        }

        void Append(TArrayRef<const TSparseRow> items, TTFInt batchSize) {
            Append(items.begin(), items.end(), batchSize);
        }

        template <class TIter>
        void Append(TIter begin, TIter end, TTFInt batchSize) {
            Y_ENSURE_EX(begin < end,
                TTFException() << "empty sequence given"
            );

            if (batchSize == 0) {
                AppendBatch(begin, end);
            } else {
                TTFInt samples = TTFInt(end - begin);
                TTFInt numBatches = samples / batchSize;
                for (; numBatches; --numBatches, begin += batchSize) {
                    AppendBatch(begin, begin + batchSize);
                }
                if (begin < end) {
                    AppendBatch(begin, end);
                }
            }
        }

        void AppendBatch(TArrayRef<const TSparseRow> items) {
            return AppendBatch(items.begin(), items.end());
        }

        template<typename TIter>
        void AppendBatch(TIter begin, TIter end) {
            Y_ENSURE_EX(begin < end,
                TTFException() << "empty sequence given"
            );
            TTFInt samples = TTFInt(end - begin);
            TTFInt nonZeros = 0;
            for (auto it = begin; it < end; ++it) {
                nonZeros += it->Data.size();
            }

            TTFTensor shape(ETFDataType::DT_INT64, TTFTensorShape{2,});
            auto eigenShape = shape.template tensor<TTFInt, 1>();
            eigenShape(0) = samples;
            eigenShape(1) = SampleSize;

            TTFTensor values(
                DataTypeByCppType<TDataType>(),
                TTFTensorShape{nonZeros,});
            TDataType* eigenValues = values.template flat<TDataType>().data();

            TTFTensor indices(
                ETFDataType::DT_INT64,
                TTFTensorShape{nonZeros, 2});
            auto eigenIndices = indices.template tensor<TTFInt, 2>();

            auto it = begin;
            TTFInt index = 0;
            TTFInt cumulativeNumSamples = 0;
            for (TTFInt i = 0; i < samples; ++i, ++it) {
                Y_ENSURE_EX(
                    it->Data.size() == it->ColumnIndices.size(),
                    TTFException()
                        << "given " << it->Data.size() << " values, but "
                        << it->ColumnIndices.size() << " column indices"
                );
                MemCopy(
                    eigenValues + cumulativeNumSamples,
                    it->Data.data(),
                    it->Data.size()
                );
                cumulativeNumSamples += it->Data.size();

                for (size_t j = 0; j < it->ColumnIndices.size(); ++j) {
                    eigenIndices(index + j, 0) = i;
                    eigenIndices(index + j, 1) = it->ColumnIndices[j];
                }
                index += it->ColumnIndices.size();
            }

            Batches.emplace_back(samples, TVector<TTFTensor>{indices, values, shape});
            NumSamples += samples;
        }

    private:
        TTFInt SampleSize = 0;
        TTFInt NumSamples = 0;

        TVector<TBatch> Batches;
    };

    class ITFInputNode {
    public:
        virtual TVector<pair<TString, TTFTensor>> Get() const = 0;
        virtual void Next() = 0;
        virtual TTFInt GetNumBatches() const = 0;
        virtual bool Valid() const = 0;
    };

    class TTFDynamicInputNode: public ITFInputNode {
    public:
        TTFDynamicInputNode() = delete;

        TTFDynamicInputNode(TTFInt size) {
            Y_ENSURE_EX(size > 0,
                TTFException() << "calling ctor with zero size"
            );
            Inputs.resize(size, nullptr);
            Names.resize(size);
        }

        TTFDynamicInputNode(std::initializer_list<pair<TString, const ITFInput*>> inputs) {
            Inputs.resize(inputs.size(), nullptr);
            Names.resize(inputs.size());
            Assign(inputs);
        }

        TTFDynamicInputNode& Set(size_t index, pair<TString, const ITFInput*> input) {
            Y_ENSURE_EX(
                index < Inputs.size(),
                TTFException()
                    << "input index " << index
                    << " out of range [0; " << Inputs.size() << ")"
            );
            Y_ENSURE_EX(
                input.second != nullptr,
                TTFException()
                    << "trying to set input at index " << index
                    << " with nullptr"
            );
            Y_ENSURE_EX(
                Inputs[index] == nullptr,
                TTFException()
                    << "trying to replace non-nullptr input at index " << index
            );
            Inputs[index] = input.second;
            Names[index] = input.first;
            return *this;
        }

        template <class TIter>
        TTFDynamicInputNode& Assign(TIter begin, TIter end) {
            size_t size = end - begin;
            Y_ENSURE_EX(size > 0,
                TTFException() << "trying to assign with zero size list"
            );
            Y_ENSURE_EX(
                Inputs.size() == size,
                TTFException()
                    << "trying to assign " << Inputs.size()
                    << " inputs with " << size << " values"
            );

            TTFInt i = 0;
            for (; begin < end; ++begin, ++i) {
                Names[i] = begin->first;
                Inputs[i] = begin->second;
            }
            return *this;
        }

        TTFDynamicInputNode& Assign(std::initializer_list<pair<TString, const ITFInput*>> inputs) {
            return Assign(inputs.begin(), inputs.end());
        }

        TTFInt GetNumBatches() const override {
            if (Inputs.empty()) {
                Y_ASSERT(false);
                return 0;
            }
            Y_ENSURE_EX(Inputs[0] != nullptr,
                TTFException() << "pointer to input " << 0 << " is nullptr"
            );
            return Inputs[0]->GetNumBatches();
        }

        bool Valid() const override {
            return Index < GetNumBatches();
        }

        TVector<pair<TString, TTFTensor>> Get() const override {
            TVector<pair<TString, TTFTensor>> batchData;
            TMaybe<TTFInt> batchSize;

            if (Inputs.empty()) {
                Y_ASSERT(false);
                return {};
            }

            for (size_t i = 0; i < Inputs.size(); ++i) {
                Y_ENSURE_EX(Inputs[i] != nullptr,
                    TTFException() << "pointer to input " << i << " is nullptr"
                );
                Y_ENSURE_EX(
                    Index < Inputs[i]->GetNumBatches(),
                    TTFException()
                        << "input " << i << " has only " << Inputs[i]->GetNumBatches()
                        << " batches, trying to get batch " << Index
                );

                TVector<TString> names = Inputs[i]->GetInputNames(Names[i]);
                TBatch batch = Inputs[i]->GetBatch(Index);

                Y_ENSURE_EX(
                    names.size() == batch.Tensors.size(),
                    TTFException()
                        << "input " << i
                        << " returned " << names.size() << " names of input tensors"
                        << ", but " << batch.Tensors.size() << " tensors"
                );

                if (batchSize.Empty()) {
                    batchSize = batch.NumSamples;
                } else {
                    Y_ENSURE_EX(
                        batch.NumSamples == batchSize,  // check current consistency
                        TTFException()
                            << "inconsistent size of batch: input 0 has " << batchSize
                            << " samples in batch, while input " << i
                            << " has " << batch.NumSamples << " samples in batch"
                    );
                }
                for (size_t j = 0; j < names.size(); ++j) {
                    batchData.emplace_back(names[j], batch.Tensors[j]);
                }
            }
            return batchData;
        };

        void Next() override {
            if (Index < GetNumBatches()) {
                ++Index;
            } else {
                Y_ENSURE(false);
            }
        };

        void Reset() {
            Index = 0;
        }

    private:
        TTFInt Index = 0;
        TVector<const ITFInput*> Inputs;
        TVector<TString> Names;
    };

    template <typename... TInTypes>
    class TTFInputNode: public ITFInputNode {
    public:
        using TInputSignature = TTFInputSignature<TInTypes...>;
        using TSelf = TTFInputNode<TInTypes...>;

        template <TTFInt Index>
        using TInputTypeByIndex = typename TInputSignature::template TByIndex<Index>::TResult;

        TTFInputNode()
            : Impl(TInputSignature::Size)
        {
        }

        TTFInputNode(pair<TString, TInTypes*>... inputs)
            : Impl(TInputSignature::Size)
        {
            Assign(inputs...);
        }

        template <size_t Index>
        TSelf& Set(pair<TString, TInputTypeByIndex<Index>*> input) {
            static_assert(Index < TInputSignature::Size, "");
            Impl = Impl.Set(Index, input);
            return *this;
        }

        TSelf& Assign(pair<TString, TInTypes*>... inputs) {
            Impl = Impl.Assign({inputs...});
            return *this;
        }

        TTFInt GetNumBatches() const override {
            return Impl.GetNumBatches();
        }

        bool Valid() const override {
            return Impl.Valid();
        }

        TVector<pair<TString, TTFTensor>> Get() const override {
            return Impl.Get();
        };

        void Next() override {
            Impl.Next();
        };

        void Reset() {
            Impl.Reset();
        }

    private:
        TTFDynamicInputNode Impl;
    };
}
