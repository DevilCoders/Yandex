#include "states.h"

#include "tokenizer.h"

#include "saveload_utils.h"

#include <util/generic/cast.h>
#include <util/charset/unidata.h>
#include <util/stream/str.h>
#include <util/string/cast.h>
#include <util/generic/hash.h>
#include <util/generic/xrange.h>
#include <util/stream/output.h>
#include <library/cpp/deprecated/split/split_iterator.h>
#include <cmath>

#ifdef _sse_
#include <library/cpp/vec4/vec4.h>
#endif

namespace NNeuralNetApplier {

TCharMatrix::TCharMatrix()
    : NumRows(0)
    , NumColumns(0)
    , BeginPointer(nullptr)
{
}

const ui32 MAX_ABS_CHAR_VAL = (1 << 8) - 1;

TVector<float> GetUniformBucketValues(float minValue, float maxValue, ui32 numBits) {
    float coef = (maxValue - minValue) / (MAX_ABS_CHAR_VAL + 1);
    TVector<float> result;
    const size_t max = 1ull << numBits;
    for (size_t cur = 0; cur < max; ++cur) {
        result.push_back((cur + 0.5) * coef + minValue);
    }
    return result;
}

ui8 FloatToChar(float f, float minValue, float maxValue) {
    if (f < minValue) {
        f = minValue;
    }
    if (f > maxValue) {
        f = maxValue;
    }
    float ratio = (f - minValue) / (maxValue - minValue);
    if (ratio == 1.0) {
        return MAX_ABS_CHAR_VAL;
    }
    return ratio * (MAX_ABS_CHAR_VAL + 1);
}

TCharMatrix::TCharMatrix(size_t numRows, size_t numColumns, const TVector<float>& bucketValues)
    : BucketValues(bucketValues)
{
    Resize(numRows, numColumns);
}

void TCharMatrix::Resize(size_t numRows, size_t numColumns) {
    if (NumRows == numRows && NumColumns == numColumns) {
        return;
    }
    NumRows = numRows;
    NumColumns = numColumns;
    Data.resize(NumRows * NumColumns);
    BeginPointer = Data.data();
}

void TCharMatrix::GetRow(size_t row, TVector<float>& result) {
    result.resize(NumColumns);
    ui8* matrixRow = BeginPointer + row * NumColumns;
    for (size_t i = 0; i < NumColumns; ++i) {
        result[i] = BucketValues[matrixRow[i]];
    }
}

namespace {
    void MultiplyAndAddRowToImpl(const float* dict, const ui8* indices, size_t nCols, float factor, float* output) {
        size_t i = 0;
#ifdef _sse_
        if (nCols >= 4) {
            TVec4f factorVec(factor);
            for (; i + 4 <= nCols; i += 4) {
                TVec4f rowVec(
                    dict[indices[i]],
                    dict[indices[i + 1]],
                    dict[indices[i + 2]],
                    dict[indices[i + 3]]
                );
                rowVec *= factorVec;
                TVec4f outVec(output + i);
                outVec += rowVec;
                outVec.Store(output + i);
            }
        }
#endif
        for (; i < nCols; ++i) {
            output[i] += dict[indices[i]] * factor;
        }
    }
}

void TCharMatrix::MutliplyAndAddRowTo(size_t row, float value, float* outputRow) {
    const ui8* matrixRow = BeginPointer + row * NumColumns;
    MultiplyAndAddRowToImpl(BucketValues.data(), matrixRow, NumColumns, value, outputRow);
}

void TCharMatrix::Save(IOutputStream* s) const {
    THashMap<TString, TString> fields;
    fields["NumRows"] = SaveToStroka(NumRows);
    fields["NumColumns"] = SaveToStroka(NumColumns);
    fields["BucketValues"] = SaveToStroka(BucketValues);
    fields["NumUnpackedColumns"] = SaveToStroka(NumUnpackedColumns);
    TString fieldsString = SaveToStroka(fields);

    TString matrixString = DataBlob();

    SaveVectorStrok(s, { fieldsString, matrixString });
}

TString TCharMatrix::DataBlob() const {
    TStringStream ss;

    ui8 *arr = BeginPointer;
    size_t len = NumRows * NumColumns;
    for (size_t i = 0; i < len; ++i) {
        ::Save(&ss, arr[i]);
    }
    return TString(ss.Data(), ss.Size());
}

size_t TCharMatrix::Load(const TBlob& blob) {
    TBlob curBlob = blob;
    size_t totalLength = ReadSize(curBlob);
    curBlob = curBlob.SubBlob(0, totalLength);

    auto fields = ReadFields(curBlob);
    LoadFromStroka<size_t>(fields.at("NumRows"), &NumRows);
    LoadFromStroka<size_t>(fields.at("NumColumns"), &NumColumns);
    LoadFromStroka<TVector<float>>(fields.at("BucketValues"), &BucketValues);

    if (fields.contains("IsUniformPacked")) {
        LoadFromStroka<bool>(fields.at("IsUniformPacked"), &IsUniform);
        if (IsUniform) {
            float uniformPackedMax = 0;
            float uniformPackedMin = 0;
            LoadFromStroka<float>(fields.at("UniformPackedMax"), &uniformPackedMax);
            LoadFromStroka<float>(fields.at("UniformPackedMin"), &uniformPackedMin);
            ReconstructedMin = uniformPackedMin;
            ReconstructedCoeff = (uniformPackedMax - uniformPackedMin) / (MAX_ABS_CHAR_VAL + 1);
        }
    } else {
        if (BucketValues.size() > 1) {
            float maxDiff = 0;
            float minDiff = BucketValues[1] - BucketValues[0];
            for(auto i : xrange<size_t>(1, BucketValues.size())) {
                maxDiff = Max(maxDiff, BucketValues[i] - BucketValues[i-1]);
                minDiff = Min(minDiff, BucketValues[i] - BucketValues[i-1]);
            }
            IsUniform = (1.f - minDiff/maxDiff) < 0.01;
            if (IsUniform) {
                //we solve system of equations: to reproduce min/max from input of GetUniformBucketValues
                ReconstructedCoeff = 1. / 255 * (BucketValues.back() - BucketValues.front());
                ReconstructedMin = BucketValues.front() - 1. / 2 / 255 * (BucketValues.back() - BucketValues.front());
            }
        }
    }
    if (fields.contains("NumUnpackedColumns")) {
        LoadFromStroka<ui64>(fields.at("NumUnpackedColumns"), &NumUnpackedColumns);
    } else {
        NumUnpackedColumns = 0;
    }

    TBlob matrixBlob = ReadBlob(curBlob);
    Y_ASSERT(matrixBlob.Size() == NumRows * NumColumns * sizeof(ui8));

    BeginPointer = (ui8*)matrixBlob.Begin();

    Y_ASSERT(curBlob.Empty());

    return totalLength + sizeof(size_t);
}

template <class T>
TGenericMatrix<T>::TGenericMatrix(size_t numRows, size_t numColumns)
    : NumRows(0)
    , NumColumns(0)
    , BeginPointer(nullptr)
{
    Resize(numRows, numColumns);
}

template <class T>
TGenericMatrix<T>::TGenericMatrix(size_t numRows, size_t numColumns, T defaultValue)
    : NumRows(0)
    , NumColumns(0)
    , BeginPointer(nullptr)
{
    Resize(numRows, numColumns);
    Data.assign(Data.size(), defaultValue);
}

template <class T>
TGenericMatrix<T>::TGenericMatrix()
    : NumRows(0)
    , NumColumns(0)
    , BeginPointer(nullptr)
{
}

template <class T>
TGenericMatrix<T>::TGenericMatrix(const TGenericMatrix& other)
    : Data(other.Data)
    , NumRows(other.NumRows)
    , NumColumns(other.NumColumns)
{
    if (other.OwnsData()) {
        BeginPointer = Data.data();
    } else {
        BeginPointer = other.BeginPointer;
    }
}

template <class T>
TGenericMatrix<T>::TGenericMatrix(TGenericMatrix&& other) noexcept
    : Data(std::move(other.Data))
    , NumRows(other.NumRows)
    , NumColumns(other.NumColumns)
{
    if (other.OwnsData()) {
        BeginPointer = Data.data();
    } else {
        BeginPointer = other.BeginPointer;
    }
}

template <class T>
TGenericMatrix<T>& TGenericMatrix<T>::operator=(const TGenericMatrix<T>& other) {
    NumRows = other.NumRows;
    NumColumns = other.NumColumns;
    if (other.OwnsData()) {
        Data = other.Data;
        BeginPointer = Data.data();
    } else {
        BeginPointer = other.BeginPointer;
        Data.clear();
    }
    return *this;
}

template <class T>
TGenericMatrix<T>& TGenericMatrix<T>::operator=(TGenericMatrix<T>&& other) noexcept {
    NumRows = other.NumRows;
    NumColumns = other.NumColumns;
    if (other.OwnsData()) {
        Data = std::move(other.Data);
        BeginPointer = Data.data();
    } else {
        BeginPointer = other.BeginPointer;
        Data.clear();
    }
    return *this;
}

template <class T>
void TGenericMatrix<T>::Save(IOutputStream* s) const {
    Save(s, 0);
}

template <class T>
void TGenericMatrix<T>::Save(IOutputStream* s, size_t blobOffset) const {
    THashMap<TString, TString> fields;
    fields["NumRows"] = SaveToStroka(NumRows);
    fields["NumColumns"] = SaveToStroka(NumColumns);
    TString fieldsString = SaveToStroka(fields);

    // We want to save matrix as follows:
    // size_t totalLenght
    // size_t fieldsLength
    // char* fieldsString
    // size_t matrixLength
    // char* matrixData, we add a zero prefix to matrixData in order to
    // align matrix properly.
    size_t matrixOffset = blobOffset + sizeof(size_t) * 3 + fieldsString.size();
    TString matrixString = DataBlob(matrixOffset);

    SaveVectorStrok(s, { fieldsString, matrixString });
}

template <class T>
size_t TGenericMatrix<T>::Load(const TBlob& blob) {
    TBlob curBlob = blob;
    size_t totalLength = ReadSize(curBlob);
    curBlob = curBlob.SubBlob(0, totalLength);

    auto fields = ReadFields(curBlob);
    LoadFromStroka<size_t>(fields.at("NumRows"), &NumRows);
    LoadFromStroka<size_t>(fields.at("NumColumns"), &NumColumns);

    TBlob matrixBlob = ReadBlob(curBlob);
    size_t actualDataSize = NumRows * NumColumns * sizeof(float);

    Y_ASSERT(matrixBlob.Size() >= actualDataSize);
    Y_ASSERT(matrixBlob.Size() < actualDataSize + 64);

    // Skipping first fictive zero prefix.
    matrixBlob = matrixBlob.SubBlob(matrixBlob.Size() - actualDataSize, matrixBlob.Size());

    Y_ASSERT(curBlob.Empty());

    BeginPointer = (T*)matrixBlob.Begin();

    return totalLength + sizeof(size_t);
}

template <class T>
void TGenericMatrix<T>::Resize(size_t numRows, size_t numColumns) {
    if (NumRows == numRows && NumColumns == numColumns) {
        return;
    }
    Y_VERIFY(OwnsData());
    NumRows = numRows;
    NumColumns = numColumns;
    Data.resize(NumRows * NumColumns);
    BeginPointer = Data.data();
}

template <class T>
void TGenericMatrix<T>::FillZero() {
    for (auto i: xrange(+GetSize())) {
        BeginPointer[i] = 0;
    }
}

template <class T>
TString TGenericMatrix<T>::DataBlob(size_t matrixOffset) const {
    TStringStream ss;

    // Here we add a fictive zero prefix in order to aligne actual data.
    while ((matrixOffset + ss.Size()) % 64 != 0) {
        char zero = 0;
        ::Save(&ss, zero);
    }

    const T *arr = (*this)[0];
    size_t len = NumRows * NumColumns;
    for (size_t i = 0; i < len; ++i) {
        ::Save(&ss, arr[i]);
    }
    return TString(ss.Data(), ss.Size());
}

namespace {
    template <class T>
    TString TypeToString() {
        T::this_is_unsupported_type;
        return "";
    }

    template <>
    TString TypeToString<ui64>() {
        return "ui64";
    }

    template <>
    TString TypeToString<i64>() {
        return "int64";
    }

    template <>
    TString TypeToString<float>() {
        return "float";
    }

    template <>
    TString TypeToString<double>() {
        return "double";
    }
}
template <class T>
TString TGenericMatrix<T>::GetTypeName() const {
    return "TMatrix<" + TypeToString<T>() + ">";
}

template <class T>
void TGenericMatrix<T>::AcquireData() {
    if (!OwnsData()) {
        Data.assign(BeginPointer, BeginPointer + NumRows * NumColumns);
        BeginPointer = Data.data();
    }
}

static float CutFractionBorderElements(const float* a, size_t len, float sign, float fraction) {
    Y_VERIFY(fraction <= 1);
    size_t needElements = len * fraction;
    if (needElements == 0) {
        needElements = 1;
    }

    if (len == 0) {
        return -1e30;
    }

    float threshold = -1e30;
    TVector<float> v(Reserve(needElements * 2));
    for (size_t i = 0; i < len; ++i) {
        if (v.size() == 2 * needElements) {
            NthElement(v.begin(), v.begin() + needElements - 1, v.end(),
                [](float l, float r){return l > r;}
            );
            v.resize(needElements);
            threshold = v.back();
        }

        float curValue = a[i] * sign;
        if (curValue > threshold) {
            v.push_back(curValue);
        }
    }
    NthElement(v.begin(), v.begin() + needElements - 1, v.end(),
        [](float l, float r){return l > r;}
    );
    Y_VERIFY(v.size() >= needElements);
    v.resize(needElements);
    return sign * v[needElements - 1];
}

void TMatrix::SaveAsCharMatrix(IOutputStream* s, double comressQuantile, float deletionPercent) const {
    THashMap<TString, TString> fields;
    fields["NumRows"] = SaveToStroka(NumRows);
    fields["NumColumns"] = SaveToStroka(NumColumns);

    const float *arr = (*this)[0];
    size_t len = NumRows * NumColumns;

    // For UTA we use 0.00000001
    TCompressionInfo compressionInfo;
    compressionInfo.MinBorder = CutFractionBorderElements(arr, len, -1, comressQuantile);
    compressionInfo.MaxBorder = CutFractionBorderElements(arr, len, +1, comressQuantile);
    compressionInfo.DeletionPercent = deletionPercent;
    compressionInfo.GlobalAvg = std::accumulate(arr, arr + len, 0.f) / len;

    fields["BucketValues"] = SaveToStroka(GetUniformBucketValues(compressionInfo.MinBorder, compressionInfo.MaxBorder, 8));
    fields["DeletedValueRestoreResult"] = SaveToStroka(compressionInfo.GlobalAvg);
    fields["IsUniformPacked"] = SaveToStroka(true);
    fields["UniformPackedMin"] = SaveToStroka(compressionInfo.MinBorder);
    fields["UniformPackedMax"] = SaveToStroka(compressionInfo.MaxBorder);
    TString fieldsString = SaveToStroka(fields);

    TString matrixString = CharDataBlob(compressionInfo);

    SaveVectorStrok(s, { fieldsString, matrixString });
}

void TMatrix::Randomize() {
    for (auto i : xrange(GetSize())) {
        BeginPointer[i] = rand() * 1.0 / (1 << 30);
    }
}

TString TMatrix::CharDataBlob(TCompressionInfo compressionInfo) const {
    const float *arr = (*this)[0];
    size_t len = NumRows * NumColumns;
    size_t maxDeleted = NumColumns * compressionInfo.DeletionPercent / 100;
    ui8 newVal = FloatToChar(compressionInfo.GlobalAvg, compressionInfo.MinBorder, compressionInfo.MaxBorder);

    TStringStream ss;
    if (compressionInfo.DeletionPercent > 0) {
        for(auto i : xrange(NumRows)) {
            float cutBorder = 0;
            {
                TVector<float> copy((*this)[i], (*this)[i] + NumColumns);
                for(float& x : copy) {
                    x = x - compressionInfo.GlobalAvg;
                    x *= x;
                }
                Sort(copy);
                cutBorder = copy[NumColumns * compressionInfo.DeletionPercent / 100];
            }
            size_t deleted = 0;
            for(auto j : xrange(NumColumns)) {
                float val = (*this)[i][j];
                ui8 curValue = FloatToChar(val, compressionInfo.MinBorder, compressionInfo.MaxBorder);
                float diffsq = val - compressionInfo.GlobalAvg;
                diffsq *= diffsq;
                if (deleted < maxDeleted && diffsq <= cutBorder) {
                    curValue = newVal;
                    deleted += 1;
                }
                ::Save(&ss, curValue);
            }
        }
    } else {
        for (size_t i = 0; i < len; ++i) {
            ui8 curValue = FloatToChar(arr[i], compressionInfo.MinBorder, compressionInfo.MaxBorder);
            ::Save(&ss, curValue);
        }
    }

    return TString(ss.Data(), ss.Size());
}

TString TContext::DataBlob(const TVector<TString>& compressMatrixes,
    size_t blobOffset, double comressQuantile, float deletionPercent) const
{
    TStringStream s;
    ::Save(&s, (ui64)Data.size());
    for (auto& it : Data) {
        TString variableName = it.first;
        ::Save(&s, variableName.size());
        s << variableName;

        bool needToCompressMatrix = true;
        TString currentTypeName = it.second->GetTypeName();
        if (currentTypeName != "TMatrix") {
            needToCompressMatrix = false;
        }
        if (Find(compressMatrixes.begin(), compressMatrixes.end(), it.first)
            == compressMatrixes.end())
        {
            needToCompressMatrix = false;
        }

        TString resultTypeName = currentTypeName;
        if (needToCompressMatrix) {
            resultTypeName = "TCharMatrix";
        }

        ::Save(&s, resultTypeName.size());
        s << resultTypeName;
        if (it.second->GetTypeName() == "TMatrix") {
            TMatrix* matrix = VerifyDynamicCast<TMatrix*>(it.second.Get());
            if (!needToCompressMatrix) {
                matrix->Save(&s, blobOffset + s.Size());
            } else {
                matrix->SaveAsCharMatrix(&s, comressQuantile, deletionPercent);
            }
        } else {
            it.second->Save(&s);
        }
    }
    return TString(s.Data(), s.Size());
}

void TContext::Save(IOutputStream* s, const TVector<TString>& compressMatrixes,
    size_t blobOffset, double comressQuantile, float deletionPercent) const
{
    TString dataBlob = DataBlob(compressMatrixes, blobOffset + sizeof(size_t), comressQuantile, deletionPercent);
    ::Save(s, (ui64)dataBlob.size());
    *s << dataBlob;
}

IStatePtr CreateState(const TString& typeName) {
    IStatePtr state = nullptr;
    if (typeName == "TMatrix") {
        state = new TMatrix();
    } else if (typeName == "TMatrix<uint64>") {
        state = new TGenericMatrix<ui64>();
    } else if (typeName == "TMatrix<int64>") {
        state = new TGenericMatrix<i64>();
    } else if (typeName == "TMatrix<float>") {
        state = new TGenericMatrix<float>();
    } else if (typeName == "TMatrix<double>") {
        state = new TGenericMatrix<double>();
    } else if (typeName == "TCharMatrix") {
        state = new TCharMatrix();
    } else if (typeName == "TSparsifier") {
        state = new TSparsifier();
    } else if (typeName == "TSparseMatrix") {
        state = new TSparseMatrix();
    } else if (typeName == "TSamplesVector") {
        state = new TSamplesVector();
    } else if (typeName == "TTextsVector") {
        state = new TTextsVector();
    } else if (typeName == "TGlobalSparsifier") {
        state = new TGlobalSparsifier();
    } else {
        ythrow yexception() << "Unknown IState implementation type: "
            << typeName;
    }
    return state;
}

void TContext::Load(TBlob& blob, const TLoadParams& loadParams) {
    Data.clear();

    ui64 totalLenth = ReadSize(blob);

    blob = blob.SubBlob(0, totalLenth);
    size_t size = ReadSize(blob);
    Data.reserve(size * 2);

    for (size_t i = 0; i < size; ++i) {
        TString name = ReadString(blob);
        TString typeName = ReadString(blob);
        auto state = CreateState(typeName);
        size_t stateLength = state->Load(blob, loadParams);
        blob = blob.SubBlob(stateLength, blob.Size());
        Data[name] = state;
    }
}

void TContext::RenameVariable(const TString& name, const TString& newName) {
    if (!Data.contains(name)) {
        return;
    }
    Y_VERIFY(!Data.contains(newName));
    Data[newName] = Data.at(name);
    Data.erase(name);
}

void TContext::RemoveVariable(const TString& name) {
    Y_ENSURE(Data.contains(name), "Trying to remove variable " + name + ", which does not exist");
    Data.erase(name);
}

TVector<TString> TContext::GetVariables() const {
    TVector<TString> result;
    for (const auto& it : Data) {
        result.push_back(it.first);
    }
    return result;
}

template class TGenericMatrix<float>;
template class TGenericMatrix<double>;
template class TGenericMatrix<i64>;
template class TGenericMatrix<ui64>;

}

template<>
void Out<NNeuralNetApplier::TPositionedOneHotEncoding>(IOutputStream& o, const NNeuralNetApplier::TPositionedOneHotEncoding& enc) {
    o << enc.Index << ' ' << size_t(enc.Type) << ' ' << enc.Region.Begin << ' ' << enc.Region.End;
}
