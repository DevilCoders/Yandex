#include "factor_borders.h"
#include "factor_slices.h"

#include <kernel/factors_info/factors_info.h>
#include <kernel/feature_pool/feature_pool.h>

#include <util/stream/str.h>

namespace {
    // Validates one subtree with root at slice
    static bool ValidateBordersImpl(const NFactorSlices::TFactorBorders& borders,
        NFactorSlices::EFactorSlice slice)
    {
        using namespace NFactorSlices;

        const TSliceOffsets& offsets = borders[slice];

        if (offsets.Begin > offsets.End) { // Invalid offset
            return false;
        }

        const TSlicesInfo* info = GetSlicesInfo();

        if (info->IsLeaf(slice)) { // Leaf is always valid
            return true;
        }

        TSliceOffsets recOffsets;
        for (TSiblingIterator iter(slice); iter.Valid(); iter.Next()) {
            if (!ValidateBordersImpl(borders, *iter)) { // Validate sub-tree
                return false;
            }

            const TSliceOffsets& partOffsets = borders[*iter];
            if (recOffsets.Empty()) {
                recOffsets = partOffsets;
            } else if (!partOffsets.Empty()) {
                if (recOffsets.End != partOffsets.Begin) { // Child slices should be adjacent
                    return false;
                }
                recOffsets.End = partOffsets.End;
            }
        }

        if (offsets == recOffsets) { // Parent should be equal to union of children
            return true;
        }

        return false;
    }

    static bool ReConstructBordersImpl(const NFactorSlices::TFactorBorders& borders,
        NFactorSlices::EFactorSlice slice,
        NFactorSlices::TSlicesMetaInfo& recMetaInfo,
        NFactorSlices::TFactorBorders& recBorders,
        const NFactorSlices::TSliceOffsets& offsets,
        const NFactorSlices::NDetail::TReConstructOptions& options)
    {
        using namespace NFactorSlices;

        if (offsets.Begin > offsets.End) {
            return false;
        }

        const TSlicesInfo* info = GetSlicesInfo();

        if (info->IsLeaf(slice)) {
            recMetaInfo.SetNumFactors(slice, offsets.Size());
            recMetaInfo.SetSliceEnabled(slice, !offsets.Empty());
            recBorders[slice] = offsets;
            return true;
        }

        EFactorSlice child = info->GetFirstChild(slice);
        if (info->HasOneChild(slice) && borders[child].Empty()) {
            if (ReConstructBordersImpl(borders, child, recMetaInfo, recBorders, offsets, options)) {
                recMetaInfo.SetSliceEnabled(slice, !offsets.Empty());
                recBorders[slice] = offsets;
                return true;
            }
            return false;
        }

        TSliceOffsets recOffsets;
        for (TSiblingIterator iter(slice); iter.Valid(); iter.Next()) {
            const TSliceOffsets& recPartOffsets = recBorders[*iter];
            if (!ReConstructBordersImpl(borders, *iter,
                recMetaInfo, recBorders, borders[*iter], options))
            {
                return false;
            }

            if (recOffsets.Empty()) {
                recOffsets = recPartOffsets;
            } else if (!recPartOffsets.Empty()) {
                if (recOffsets.End != recPartOffsets.Begin) {
                    return false;
                }
                recOffsets.End = recPartOffsets.End;
            }
        }

        if (options.IgnoreHierarchicalBorders || offsets.Empty() || offsets == recOffsets) {
            recMetaInfo.SetSliceEnabled(slice, !recOffsets.Empty());
            recBorders[slice] = recOffsets;
            return true;
        }

        return false;
    }
}

namespace NFactorSlices {
    bool TFactorBorders::TryToValidate() const
    {
        return ::ValidateBordersImpl(*this, EFactorSlice::ALL) && Get(EFactorSlice::ALL).Begin == 0;
    }

    void TFactorBorders::Erase(const TSliceOffsets& offsets)
    {
        auto& borders = *this;
        for (TSliceIterator iter; iter.Valid(); iter.Next()) {
            borders[*iter].Erase(offsets);
        }
    }

    bool TFactorBorders::IsMinimal(const TSliceOffsets& offsets) const
    {
        auto& borders = *this;
        for (TSliceIterator iter; iter.Valid(); iter.Next()) {
            if (!borders[*iter].Empty() && offsets.Contains(borders[*iter])) {
                return false;
            }
        }
        return true;
    }

    size_t TFactorBorders::SizeAll() const
    {
        return (*this)[EFactorSlice::ALL].Size();
    }

    TVector<EFactorSlice> GetFactorSlices(
        const TFactorBorders& borders,
        ESerializationMode mode)
    {
        NMLPool::TFeatureSlice rawSlices;
        const TSlicesInfo* info = GetSlicesInfo();
        TVector<EFactorSlice> factorSlices;
        for (EFactorSlice slice : GetAllFactorSlices()) {
            if (borders[slice].Empty())
                continue;

            if (ESerializationMode::LeafOnly == mode && !info->IsLeaf(slice)) {
                continue;
            }

            factorSlices.push_back(slice);
        }
        return factorSlices;
    }

    void SerializeFactorBorders(
        IOutputStream& out,
        const TFactorBorders& borders,
        ESerializationMode mode)
    {
        NMLPool::TFeatureSlices rawSlices;
        for (EFactorSlice slice: GetFactorSlices(borders, mode)) {
            rawSlices.emplace_back(ToString(slice), borders[slice].Begin, borders[slice].End);
        }
        out << rawSlices;
    }

    TString SerializeFactorBorders(const TFactorBorders& borders,
        ESerializationMode mode)
    {
        TStringStream resultStream;
        SerializeFactorBorders(resultStream, borders, mode);
        return resultStream.Str();
    }

    bool TryToDeserializeFactorBorders(const TStringBuf buf, TFactorBorders& res)
    {
        try {
            DeserializeFactorBorders(buf, res);
        } catch (NFactorSlices::NDetail::TDeserializeError& /* error */) {
            return false;
        }
        return true;
    }

    static bool IsKnownSliceName(const TStringBuf& name) {
        EFactorSlice slice = EFactorSlice::COUNT;
        return TryFromString(name, slice);
    }

    // Throws exception if rawSlices contains unknown slice name
    static void CheckForUnknownSlices(const NMLPool::TFeatureSlices& rawSlices) {
        TMap<TString, TSliceOffsets> unknownSlices;
        for (const auto& slice : rawSlices) {
            if (!IsKnownSliceName(slice.Name))
                unknownSlices[slice.Name] = TSliceOffsets(slice.Begin, slice.End);
        }

        if (!unknownSlices.empty()) {
            TString unknownNames;
            for (auto& entry : unknownSlices) {
                if (!unknownNames.empty()) {
                    unknownNames += ", ";
                }
                unknownNames += entry.first;
            }

            ythrow NFactorSlices::NDetail::TSliceNameError(unknownSlices, rawSlices.size())
                << "failed to parse slice names: " << unknownNames;
        }
    }

    void DeserializeFeatureSlices(const TStringBuf buf, const bool skipNameValidation, NMLPool::TFeatureSlices& res) {
        if (!NMLPool::TryParseBordersStr(buf, res)) {
            ythrow NFactorSlices::NDetail::TParseError()
                << "failed to parse slice borders from: \"" << buf << "\"";
        }
        if (!skipNameValidation) {
            CheckForUnknownSlices(res);
        }
    }

    bool TryToDeserializeFeatureSlices(const TStringBuf buf, const bool skipNameValidation, NMLPool::TFeatureSlices& res) {
        try {
            DeserializeFeatureSlices(buf, skipNameValidation, res);
        } catch (NFactorSlices::NDetail::TDeserializeError& /* error */) {
            return false;
        }
        return true;
    }

    void DeserializeFactorBorders(const TStringBuf buf, TFactorBorders& res) {
        NMLPool::TFeatureSlices rawSlices;
        // This method guarantees that in case of unknown slices,
        // all known slices are still parsed completely.
        // Therefore we can't throw exception from DeserializeFeatureSlices.
        // ParseSlicesVector fills res borders and then performs the check.
        DeserializeFeatureSlices(buf, /*skipNameValidation=*/true, rawSlices);
        ParseSlicesVector(rawSlices, res);
    }

    void ParseSlicesVector(const NMLPool::TFeatureSlices& rawSlices, TFactorBorders& res) {
        for (const auto& rawSlice : rawSlices) {
            EFactorSlice slice = EFactorSlice::COUNT;
            if (TryFromString(rawSlice.Name, slice))
                res[slice] = TSliceOffsets(rawSlice.Begin, rawSlice.End);
        }
        CheckForUnknownSlices(rawSlices);
    }

    bool ValidateBordersNames(const TVector<TString>& names, TVector<TString>& unknownSliceNames) {
        unknownSliceNames.clear();
        for (const auto& name : names) {
            if (!IsKnownSliceName(name)) {
                unknownSliceNames.push_back(name);
            }
        }
        return unknownSliceNames.empty();
    }

} // NFactorSlices

bool NFactorSlices::IsBordersInSubsetRelation(const TFactorBorders& subsetCandidate, const TFactorBorders& supersetCandidate) {
    const NFactorSlices::TSlicesInfo* info = NFactorSlices::GetSlicesInfo();
    for (EFactorSlice slice : NFactorSlices::GetAllFactorSlices()) {
        if (!info->IsLeaf(slice)) {
            continue;
        }
        if (subsetCandidate[slice].Size() > supersetCandidate[slice].Size()) {
            return false;
        }
    }
    return true;
}

template<>
void Out<NFactorSlices::TSliceOffsets>(IOutputStream& os, const NFactorSlices::TSliceOffsets& offsets)
{
    os << "[" << offsets.Begin << ";" << offsets.End << ")";
}

template<>
bool TryFromStringImpl<NFactorSlices::TFactorBorders, char>(char const* data, size_t size, NFactorSlices::TFactorBorders& borders)
{
    return TryToDeserializeFactorBorders(TStringBuf(data, size), borders);
}

template<>
void Out<NFactorSlices::TFactorBorders>(IOutputStream& os, const NFactorSlices::TFactorBorders& borders)
{
    SerializeFactorBorders(os, borders);
}

namespace NFactorSlices {
    namespace NDetail {
        bool ReConstructMetaInfo(const TFactorBorders& fromBorders, TSlicesMetaInfo& toMetainfo,
            const TReConstructOptions& options)
        {
            TSlicesMetaInfo recMetaInfo;
            TFactorBorders recBorders;
            if (!::ReConstructBordersImpl(fromBorders, EFactorSlice::ALL,
                recMetaInfo, recBorders, fromBorders[EFactorSlice::ALL], options))
            {
                return false;
            }
            if (recBorders[EFactorSlice::ALL].Begin != 0) {
                return false;
            }
            toMetainfo = recMetaInfo;
            return true;
        }

        bool ReConstructBorders(const TFactorBorders& fromBorders, TFactorBorders& toBorders,
            const TReConstructOptions& options)
        {
            TSlicesMetaInfo recMetaInfo;
            TFactorBorders recBorders;
            if (!ReConstructBordersImpl(fromBorders, EFactorSlice::ALL,
                recMetaInfo, recBorders, fromBorders[EFactorSlice::ALL], options))
            {
                return false;
            }
            if (recBorders[EFactorSlice::ALL].Begin != 0) {
                return false;
            }
            toBorders = recBorders;
            return true;
        }

        bool ReConstruct(TFactorBorders& borders,
            const TReConstructOptions& options)
        {
            return ReConstructBorders(borders, borders, options);
        }

        void EnsuredReConstructMetaInfo(
            const TFactorBorders& fromBorders,
            TSlicesMetaInfo& toMetainfo,
            const TReConstructOptions& options)
        {
            Y_ENSURE_EX(ReConstructMetaInfo(fromBorders, toMetainfo, options),
                TDeserializeError{} << "failed to reconstruct");
        }

        void EnsuredReConstructBorders(
            const TFactorBorders& fromBorders,
            TFactorBorders& toBorders,
            const TReConstructOptions& options)
        {
            Y_ENSURE_EX(ReConstructBorders(fromBorders, toBorders, options),
                TDeserializeError{} << "failed to reconstruct");
        }

        void EnsuredReConstruct(TFactorBorders& borders, const TReConstructOptions& options)
        {
            Y_ENSURE_EX(ReConstruct(borders, options),
                TDeserializeError{} << "failed to reconstruct");
        }

        void TSortedFactorBorders::Prepare()
        {
            for (TSliceIterator iter ; iter.Valid(); iter.Next()) {
                Sorted.insert(std::make_pair(Borders[*iter], *iter));
            }
        }

        bool TSortedFactorBorders::TryToValidate() const
        {
            for (auto iter = Begin(); iter.Valid(); iter.Next()) {
                const TSliceOffsets& offsets = iter.GetOffsets();

                size_t totalSize = 0;
                auto nextIter = iter;
                for (nextIter.Next(); nextIter.Valid(); nextIter.Next()) {
                    if (offsets.Contains(nextIter.GetOffsets())) {
                        totalSize += nextIter.GetOffsets().Size();
                        iter = nextIter;
                    }
                }

                if (totalSize > 0 && totalSize != offsets.Size()) {
                    return false;
                }

                if (nextIter.Valid() && offsets.Overlaps(nextIter.GetOffsets())) {
                    return false;
                }
            }

            return true;
        }

        EFactorSlice TSortedFactorBorders::GetSliceByFactorIndex(TFactorIndex index) const
        {
            auto iter = Sorted.lower_bound(TSliceOffsets(index, index + 1));
            --iter;
            if (iter->first.Contains(index)) {
                return iter->second;
            }
            return EFactorSlice::COUNT;
        }
    } // NDetail
} // NFactorSlices
