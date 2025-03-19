#pragma once

#include <library/cpp/offroad/custom/null_vectorizer.h>
#include <library/cpp/offroad/custom/ui64_vectorizer.h>
#include <library/cpp/offroad/flat/flat_searcher.h>


namespace NDoom {


/**
 * Random access reader for data written with `TTypedWadWriter`.
 */
template <class WadDataReader>
class TWadChunk {
    using TSubSearcher = NOffroad::TFlatSearcher<ui64, std::nullptr_t, NOffroad::TUi64Vectorizer, NOffroad::TNullVectorizer>;
public:
    using TWadDataReader = WadDataReader;

    TWadChunk() = default;

    TWadChunk(TWadDataReader&& dataReader, const TString& signature = TString())
        : DataReader_(std::forward<TWadDataReader>(dataReader))
    {
        ResetInternal(signature);
    }

    TWadChunk(const TString& path, bool lockMemory = false, const TString& signature = TString())
        : DataReader_(path, lockMemory)
    {
        ResetInternal(signature);
    }

    size_t Size() const {
        return SubSearcher_.Size();
    }

    TBlob Read(size_t index) const {
        auto location = Location(index);

        return DataReader_.Read(location.first, location.second);
    }

    std::pair<ui64, ui64> Location(size_t index) const {
        ui64 offset = index == 0 ? 0 : SubSearcher_.ReadKey(index - 1);
        ui64 size = SubSearcher_.ReadKey(index) - offset;

        return { 8 + offset, size };
    }

    const TString& Signature() const {
        return Signature_;
    }

    const TWadDataReader& DataReader() const {
        return DataReader_;
    }

    TWadDataReader& DataReader() {
        return DataReader_;
    }

private:
    void ResetInternal(const TString& signature) {
        const ui64 size = DataReader_.Size();
        if (size == 0) {
            SubSearcher_.Reset();
            Signature_ = "";
            return;
        }

        Y_ENSURE_EX(size >= 16, yexception() << "Invalid IWAD.");

        TBlob headBlob = DataReader_.ReadHead(0, 4);
        Y_ENSURE_EX(TStringBuf(headBlob.AsCharPtr(), headBlob.Size()) == "IWAD", yexception() << "Invalid IWAD.");

        TBlob signatureBlob = DataReader_.ReadSignature(4, 4);
        Signature_ = TString(signatureBlob.AsCharPtr(), signatureBlob.Size());
        Y_ENSURE_EX(signature.empty() || Signature_ == signature, yexception() << "Unexpected IWAD type.");

        TBlob sizeBlob = DataReader_.ReadSubSize(size - 8, 8);
        Y_ENSURE_EX(sizeBlob.Size() == 8, yexception() << "Could not read IWAD subindex size.");

        ui64 subSize = ReadUnaligned<ui64>(sizeBlob.Data());
        Y_ENSURE_EX(subSize <= size - 16, yexception() << "Invalid IWAD subindex size");

        TBlob subSource = DataReader_.ReadSub(size - 8 - subSize, subSize);
        Y_ENSURE_EX(subSource.Size() == subSize, yexception() << "Could not map IWAD subindex into memory.");

        SubSearcher_.Reset(subSource);
    }


    TWadDataReader DataReader_;
    TSubSearcher SubSearcher_;
    TString Signature_;
};


} // namespace NOffroad
