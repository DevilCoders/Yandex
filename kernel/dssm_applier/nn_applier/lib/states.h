#pragma once

#include "load_params.h"

#include <util/charset/wide.h>
#include <util/generic/vector.h>
#include <util/memory/blob.h>
#include <util/generic/ptr.h>
#include <util/generic/hash.h>
#include <util/generic/cast.h>
#include <library/cpp/deprecated/split/split_iterator.h>
#include <library/cpp/containers/flat_hash/flat_hash.h>

namespace NNeuralNetApplier {

struct TCompressionInfo {
    float MinBorder = 0;
    float MaxBorder = 0;
    float DeletionPercent = 0;
    float GlobalAvg = 0;
};

class IState : public TThrRefBase {
public:
    ~IState() override = default;
    virtual void Save(IOutputStream* stream) const = 0;
    virtual size_t Load(const TBlob& blob) = 0;
    virtual size_t Load(const TBlob& blob, [[maybe_unused]] const TLoadParams& loadParams) {
        return Load(blob);
    }
    virtual TString GetTypeName() const = 0;
};
using IStatePtr = TIntrusivePtr<IState>;

IStatePtr CreateState(const TString& typeName);

class TCharMatrix : public IState {
private:
    TVector<ui8> Data;
    size_t NumRows = 0;
    size_t NumColumns = 0;
    TVector<float> BucketValues;
    ui8* BeginPointer = nullptr;
    size_t NumUnpackedColumns = 0;
    bool IsUniform = false;
    float ReconstructedMin = 0;
    float ReconstructedCoeff = 0;

private:
    TString DataBlob() const;

public:
    TCharMatrix();
    TCharMatrix(size_t numRows, size_t numColumns, const TVector<float>& bucketValues = TVector<float>());

    void Save(IOutputStream* s) const override;
    size_t Load(const TBlob& blob) override;

    void Resize(size_t numRows, size_t numColumns);

    void GetRow(size_t row, TVector<float>& result);

    void MutliplyAndAddRowTo(size_t row, float value, float* outputRow);

    void SetData(const TVector<ui8>& data) {
        Data = data;
        BeginPointer = Data.data();
    }

    bool IsUniformPacked() const {
        return IsUniform;
    }

    float GetReconstructedCoeff() const {
        return ReconstructedCoeff;
    }

    float GetReconstructedMin() const {
        return ReconstructedMin;
    }

    size_t GetSize() const {
        return NumColumns * NumRows;
    }

    const ui8* operator[] (const size_t row) const noexcept {
        return std::next(GetData(), row * NumColumns);
    }

    ui8* operator[] (const size_t row) noexcept {
        return std::next(GetData(), row * NumColumns);
    }

    const ui8* GetData() const noexcept {
        return BeginPointer;
    }

    ui8* GetData() noexcept {
        return BeginPointer;
    }


    size_t GetNumRows() const {
        return NumRows;
    }

    size_t GetNumColumns() const {
        return NumColumns;
    }

    void SetNumUnpackedColumns(size_t val) {
        NumUnpackedColumns = val;
    }

    // TCharMatrix may be a compressed version of a larger matrix,
    // This function returns the real number of columns,
    // while GetNumColumns() returns just the number of bytes per compressed row
    size_t GetNumUnpackedColumns() const {
        if (NumUnpackedColumns == 0) {
            return NumColumns;
        } else {
            return NumUnpackedColumns;
        }
    }

    TString GetTypeName() const override {
        return "TCharMatrix";
    }
};
using TCharMatrixPtr = TIntrusivePtr<TCharMatrix>;

template <class T>
class TGenericMatrix : public IState {
protected:
    TVector<T> Data;
    size_t NumRows;
    size_t NumColumns;
    T* BeginPointer;
protected:
    TString DataBlob(size_t matrixOffset) const;
    TString CharDataBlob(TCompressionInfo compressionInfo) const;

    bool OwnsData() const {
        return BeginPointer == Data.data();
    }
public:
    TGenericMatrix();
    TGenericMatrix(size_t numRows, size_t numColumns);
    TGenericMatrix(size_t numRows, size_t numColumns, T defaultValue);
    TGenericMatrix(size_t numRows, size_t numColumns, TVector<T> data)
        : Data(std::move(data))
        , NumRows(numRows)
        , NumColumns(numColumns)
        , BeginPointer(Data.data())
    {
        Y_ASSERT(Data.size() == GetFlatSize());
    }
    TGenericMatrix(const TGenericMatrix& other);
    explicit TGenericMatrix(TGenericMatrix&& other) noexcept ;

    TGenericMatrix& operator=(const TGenericMatrix& other);
    TGenericMatrix& operator=(TGenericMatrix&& other) noexcept;

    void Save(IOutputStream* s) const override;
    void Save(IOutputStream* s, size_t blobOffset) const ;
    size_t Load(const TBlob& blob) override;

    void Resize(size_t numRows, size_t numColumns);

    void FillZero();

    size_t GetSize() const {
        return NumColumns * NumRows;
    }

    T* GetData() {
        if (BeginPointer == nullptr) {
            return Data.data();
        } else {
            return BeginPointer;
        }
    }

    const T* GetData() const {
        if (BeginPointer == nullptr) {
            return Data.data();
        } else {
            return BeginPointer;
        }
    }

    size_t GetNumRows() const {
        return NumRows;
    }

    size_t GetNumColumns() const {
        return NumColumns;
    }

    size_t GetFlatSize() const {
        return NumRows * NumColumns;
    }

    T* operator[] (size_t row) {
        return BeginPointer + row * NumColumns;
    }

    const T* operator[] (size_t row) const {
        return BeginPointer + row * NumColumns;
    }

    TVector<T> RepresentAsArray() const {
        return TVector<T>(BeginPointer, BeginPointer + GetFlatSize());
    }

    TVector<T>& AsFlatArray() {
        if (!OwnsData()) {
            ythrow yexception() << "Cannot represent float* as vector in TGenericMatrix.AsFlatArray" ;
        }
        return Data;
    }

    virtual TString GetTypeName() const override;

    void AcquireData();
};
template <class T>
using TGenericMatrixPtr = TIntrusivePtr<TGenericMatrix<T>>;

class TMatrix : public TGenericMatrix<float> {
private:
    TString CharDataBlob(TCompressionInfo compressionInfo) const;

public:
    using TGenericMatrix<float>::TGenericMatrix;
    using TGenericMatrix<float>::operator=;

    void SaveAsCharMatrix(IOutputStream* s, double comressQuantile = 0, float deletionPercent = 0) const;

    void Randomize();

    TString GetTypeName() const override {
        return "TMatrix";
    }
};
using TMatrixPtr = TIntrusivePtr<TMatrix>;

class TSparseVector {
public:
    TVector<size_t> Indexes;
    TVector<float> Values;

    void Save(IOutputStream* s) const {
        Y_UNUSED(s);
        ythrow yexception() << "Not implemented.";
    }
};

class TSparseMatrix : public IState {
public:
    TVector<TSparseVector> Vectors;

    void Save(IOutputStream* s) const override {
        Y_UNUSED(s);
        ythrow yexception() << "Not implemented.";
    }

    size_t Load(const TBlob& blob) override {
        Y_UNUSED(blob);
        ythrow yexception() << "Not implemented.";
    }

    TString GetTypeName() const override {
        return "TSparseMatrix";
    }
};

class ISample {
public:
    virtual ~ISample() = default;
    virtual const TVector<TString>& GetAnnotations() const = 0;
    virtual const TVector<TString>& GetFeatures() const = 0;
    virtual TString GetTypeName() = 0;
};

class TSample : public ISample {
private:
    TVector<TString> Annotations;
    TVector<TString> Features;

public:
    TSample(const TVector<TString>& annotations, const TVector<TString>& features)
        : Annotations(annotations)
        , Features(features)
    {
    }

    const TVector<TString>& GetAnnotations() const override {
        return Annotations;
    }

    TVector<TString>& GetAnnotations() {
        return Annotations;
    }

    const TVector<TString>& GetFeatures() const override {
        return Features;
    }

    TVector<TString>& GetFeatures() {
        return Features;
    }

    TString GetTypeName() override {
        return "TSample";
    }
};

class TSamplesVector : public IState {
private:
    TVector<TAtomicSharedPtr<ISample>> Samples;

public:
    TSamplesVector() = default;

    TSamplesVector(const TVector<TAtomicSharedPtr<ISample>>& samples)
        : Samples(samples)
    {
    }

    void Save(IOutputStream* s) const override {
        Y_UNUSED(s);
        ythrow yexception() << "Not implemented.";
    }

    size_t Load(const TBlob& blob) override {
        Y_UNUSED(blob);
        ythrow yexception() << "Not implemented.";
    }

    const TVector<TAtomicSharedPtr<ISample>>& GetSamples() const {
        return Samples;
    }

    void SetSamples(const TVector<TAtomicSharedPtr<ISample>>& samples) {
        Samples = samples;
    }

    TString GetTypeName() const override {
        return "TSamplesVector";
    }
};

class TTextsVector : public IState {
public:
    TVector<TString> Texts;

    void Save(IOutputStream* s) const override {
        Y_UNUSED(s);
        ythrow yexception() << "Not implemented.";
    }

    size_t Load(const TBlob& blob) override {
        Y_UNUSED(blob);
        ythrow yexception() << "Not implemented.";
    }

    TString GetTypeName() const override {
        return "TTextsVector";
    }
};

enum class ETokenType {
    Word,
    Phrase,
    Bigram,
    Trigram,
    Prefix,
    Suffix,
    WideBigram,
    WordBpe
};

struct TPositionedOneHotEncoding {
    size_t Index;
    TSizeTRegion Region;
    ETokenType Type;
    TPositionedOneHotEncoding(size_t index, size_t begin, size_t end, ETokenType type)
        : Index(index)
        , Region(begin, end)
        , Type(type)
    {
    }
    bool operator==(const TPositionedOneHotEncoding& other) const {
        return Index == other.Index && Region == other.Region && Type == other.Type;
    }
};

using TPositionedOneHotEncodingVector = TVector<TPositionedOneHotEncoding>;

class TPositionedOneHotEncodingMatrix : public IState {
public:
    TVector<TPositionedOneHotEncodingVector> Encodings;

    void Save(IOutputStream* s) const override {
        Y_UNUSED(s);
        ythrow yexception() << "Not implemented.";
    }

    size_t Load(const TBlob& blob) override {
        Y_UNUSED(blob);
        ythrow yexception() << "Not implemented.";
    }

    TString GetTypeName() const override {
        return "TPositionedOneHotEncodingMatrix";
    }
};

using TStatesDictionary = NFH::TFlatHashMap<TString, IStatePtr>;

class TContext : public TThrRefBase {
private:
    TStatesDictionary Data;

private:
    TString DataBlob(const TVector<TString>& compressMatrixes = TVector<TString>(),
        size_t blobOffset = 0, double comressQuantile = 0, float deletionPercent = 0) const;

public:
    size_t operator+() {
        return Data.size();
    }

    void Save(IOutputStream* s,
        const TVector<TString>& compressMatrixes = TVector<TString>(),
        size_t blobOffset = 0, double comressQuantile = 0, float deletionPercent = 0) const;
    void Load(TBlob& blob, const TLoadParams& loadParams = {});

    bool has(const TString& s) const {
        return Data.contains(s);
    }

    const IStatePtr& at(const TString& s) const {
        return Data.at(s);
    }

    const TStatesDictionary& Dict() const {
        return Data;
    }

    IStatePtr& operator [](const TString& s) {
        return Data[s];
    }

    void RenameVariable(const TString& name, const TString& newName);
    void RemoveVariable(const TString& name);

    TVector<TString> GetVariables() const;
};

class TEvalContext {
private:
    const TContext* ParametersPtr;
    TStatesDictionary Data;

public:
    explicit TEvalContext(const TContext* params = nullptr, size_t estimateSize = 0)
        : ParametersPtr(params)
    {
        if (estimateSize > 0) {
            Data.reserve(estimateSize * 2);
        }
    }

    void SetParameters(const TContext* params) {
        ParametersPtr = params;
    }

    bool has(const TString& s) const {
        return Data.contains(s) || ParametersPtr != nullptr && ParametersPtr->has(s);
    }

    const IStatePtr& at(const TString& s) const {
        auto dit = Data.find(s);
        if (dit != Data.end()) {
            return dit->second;
        } else {
            Y_ENSURE(ParametersPtr, "Eval context has no key " << s);
            return ParametersPtr->at(s);
        }
    }

    IStatePtr& operator [](const TString& s) {
        return Data[s];
    }

    template <typename T>
    T* CreateOrGet(const TString& s) {
        auto inserted = Data.emplace(s, IStatePtr{});
        if (!inserted.second) {
            return VerifyDynamicCast<T*>(inserted.first->second.Get());
        }
        if (ParametersPtr != nullptr) {
            auto& pdict = ParametersPtr->Dict();
            auto pit = pdict.find(s);
            if (pit != pdict.end()) {
                Data.erase(inserted.first);
                return VerifyDynamicCast<T*>(pit->second.Get());
            }
        }
        inserted.first->second = new T;
        return static_cast<T*>(inserted.first->second.Get());
    }

};

}
