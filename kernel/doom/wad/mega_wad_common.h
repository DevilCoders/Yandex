#pragma once

#include "mega_wad_info.h"
#include "wad_lump_id.h"

#include <kernel/doom/blob_storage/chunked_blob_storage.h>

#include <library/cpp/offroad/flat/flat_ui32_searcher.h>

#include <util/generic/array_ref.h>
#include <util/string/cast.h>


namespace NDoom {

class TMegaWadCommon {
public:
    void Reset(TMegaWadInfo info) {
        Info_ = MakeAtomicShared<const TMegaWadInfo>(std::move(info));
        FillDocLumpTypeMap();
    }

    void Reset(const IChunkedBlobStorage* blobStorage, ui32 chunk);

    const TMegaWadInfo& Info() const {
        return *Info_;
    }

    void MapDocLumps(const TArrayRef<const TWadLumpId>& types, TArrayRef<size_t> mapping) const {
        TVector<TString> names;
        for (TWadLumpId id : types) {
            names.push_back(ToString(id));
        }

        MapDocLumps(names, mapping);
    }

    void MapDocLumps(TConstArrayRef<TString> types, TArrayRef<size_t> mapping) const {
        Y_ASSERT(types.size() == mapping.size());

        for (size_t i = 0; i < types.size(); i++) {
            TStringBuf lumpName = types[i];
            Y_ENSURE(DocLumpIndexByType_.contains(lumpName), "Wad does't have " << lumpName << " lump");
            mapping[i] = DocLumpIndexByType_.at(lumpName);
        }
    }

    void MapDocLumps(TConstArrayRef<TStringBuf> types, TArrayRef<size_t> mapping) const {
        Y_ASSERT(types.size() == mapping.size());

        for (size_t i = 0; i < types.size(); i++) {
            TStringBuf lumpName = types[i];
            Y_ENSURE(DocLumpIndexByType_.contains(lumpName), "Wad does't have " << lumpName << " lump");
            mapping[i] = DocLumpIndexByType_.at(lumpName);
        }
    }

    bool HasDocLump(size_t docLumpId) const {
        for (auto [_, lumpId] : DocLumpIndexByType_) {
            if (lumpId == docLumpId) {
                return true;
            }
        }
        return false;
    }

    void LoadDocLumpRegions(const TBlob& docBlob, const TArrayRef<const size_t>& mapping, TArrayRef<TArrayRef<const char>> regions) const {
        CommonLoadDocLumpRegions(docBlob, mapping, regions);
    }

    void LoadDocLumpRegions(const TBlob& docBlob, const TArrayRef<const size_t>& mapping, TArrayRef<TBlob> regions) const {
        CommonLoadDocLumpRegions(docBlob, mapping, regions);
    }

    template <class T>
    static void CommonLoadDocLumpRegions(const TBlob& docBlob, size_t regionsCount, const TArrayRef<const size_t>& mapping, TArrayRef<T> regions) {
        if (Y_UNLIKELY(regions.size() == 0)) {
            return;
        } else if (Y_UNLIKELY(regionsCount == 1)) {
            Y_ASSERT(mapping.size() == 0 || mapping[0] == 0);
            ConstructRegion(docBlob, 0, docBlob.Size(), regions[0]);
            return;
        }

        Y_ASSERT(mapping.size() == regions.size());

        NOffroad::TFlatUi32Searcher flatSearcher(docBlob);

        size_t headerLen = flatSearcher.FlatSize(regionsCount - 1);
        if (headerLen > docBlob.Size()) {
            ClearRegions(regions);
            return;
        }

        const char* start = docBlob.AsCharPtr() + headerLen;
        const char* end = docBlob.AsCharPtr() + docBlob.Size();
        for (size_t i = 0; i < mapping.size(); i++) {
            size_t index = mapping[i];
            Y_ASSERT(index < regionsCount);

            const char* regionStart;
            const char* regionEnd;

            if (Y_UNLIKELY(index == 0)) {
                regionStart = start;
            } else {
                regionStart = start + flatSearcher.ReadKey(index - 1);
            }

            if (Y_UNLIKELY(index == regionsCount - 1)) {
                regionEnd = end;
            } else {
                regionEnd = start + flatSearcher.ReadKey(index);
            }

            if (regionStart > end || regionStart > regionEnd || regionEnd > end) {
                ClearRegions(regions);
                return;
            }

            ConstructRegion(docBlob, regionStart - docBlob.AsCharPtr(), regionEnd - docBlob.AsCharPtr(), regions[i]);
        }
    }

private:
    void FillDocLumpTypeMap() {
        DocLumpIndexByType_.clear();

        for (size_t i = 0; i < Info_->DocLumps.size(); i++)
            DocLumpIndexByType_[Info_->DocLumps[i]] = i;
    }

    static void ConstructRegion(const TBlob& docBlob, size_t from, size_t to, TBlob& result) {
        result = docBlob.SubBlob(from, to);
    }

    static void ConstructRegion(const TBlob& docBlob, size_t from, size_t to, TArrayRef<const char>& result) {
        result = TArrayRef<const char>(docBlob.AsCharPtr() + from, docBlob.AsCharPtr() + to);
    }

    template<class T>
    static void ClearRegions(TArrayRef<T> regions) {
        for (size_t i = 0; i < regions.size(); ++i) {
            regions[i] = T();
        }
    }

    template <class T>
    void CommonLoadDocLumpRegions(const TBlob& docBlob, const TArrayRef<const size_t>& mapping, TArrayRef<T> regions) const {
        CommonLoadDocLumpRegions(docBlob, DocLumpIndexByType_.size(), mapping, regions);
    }

private:
    TAtomicSharedPtr<const TMegaWadInfo> Info_ = MakeAtomicShared<TMegaWadInfo>();

    THashMap<TString, ui32> DocLumpIndexByType_;
};


void EnableMegaWadInfoDeduplication();

} // namespace NDoom
