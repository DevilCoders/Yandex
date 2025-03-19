#pragma once

#include <kernel/search_types/search_types.h>
#include <kernel/searchlog/errorlog.h>
#include <library/cpp/offroad/key/fat_key_seeker.h>
#include <library/cpp/offroad/custom/null_vectorizer.h>
#include <library/cpp/offroad/custom/null_serializer.h>
#include <kernel/keyinv/invkeypos/keycode.h>
#include <kernel/doom/search_fetcher/search_fetcher.h>
#include <kernel/doom/offroad_wad/proto/offroad_wad_ranges.pb.h>

#include <kernel/doom/chunked_wad/single_chunked_wad.h>
#include <kernel/doom/keyinv_offroad_portions/keyinv_portions_key_ranges.h>

#include <kernel/doom/wad/wad_lump_id.h>
#include <kernel/doom/wad/deduplicator.h>
#include <util/generic/fwd.h>
#include <util/generic/yexception.h>

#include "offroad_wad_iterator.h"

namespace NDoom {

/*
 * Very simple container for wad ranges files, that checks the type of wad files — they can have single range or multiple ranges.
 * Main public methods: AddRange(...) and LoadRangeLump(...)
 */
template <EWadIndexType IndexType>
class TOffroadWadRangesLoader {
    static constexpr NDoom::TWadLumpId RangesProtoId{IndexType, NDoom::EWadLumpRole::Struct};
public:
    enum ERangesWadType {
        Unknown,
        SingleRange,
        MultiRange
    };

    TOffroadWadRangesLoader() = default;

    TOffroadWadRangesLoader(TVector<THolder<NDoom::IWad>>&& globalWadRanges) {
        for (auto &&wadRange: globalWadRanges) {
            AddWad(std::move(wadRange));
        }
    }

    TOffroadWadRangesLoader(TVector<std::pair<THolder<NDoom::IWad>, ui32>>&& globalWadRanges) {
        for (size_t i = 0; i < globalWadRanges.size(); ++i) {
            AddWad(std::move(globalWadRanges[i].first), globalWadRanges[i].second);
        }
    }

    void AddWad(THolder<NDoom::IWad>&& wad, TMaybe<ui32> wadIndex = Nothing()) {
        EnsureWadType(wad);
        if (WadType == MultiRange && wad != nullptr) {
            TBlob rangesLumpBlob = wad->LoadGlobalLump(RangesProtoId);
            NDoom::TOffroadWadRanges ranges;
            Y_PROTOBUF_SUPPRESS_NODISCARD ranges.ParseFromArray(rangesLumpBlob.data(), rangesLumpBlob.size());
            for (const auto& range: ranges.GetRanges()) {
                Y_ENSURE(!RangeWadMapping_.contains(range));
                RangeWadMapping_[range] = GlobalWadRanges_.size();
            }
        } else {
            Y_ENSURE(WadType == SingleRange || wad == nullptr);
            if (!wadIndex.Defined()) {
                wadIndex = GlobalWadRanges_.size();
            }
            RangeWadMapping_[*wadIndex] = GlobalWadRanges_.size();
        }
        GlobalWadRanges_.push_back(std::move(wad));
    }

    TVector<TBlob> LoadRangeLumps(ui32 range, const TConstArrayRef<TString> lumpIds) const {
        if (WadType == SingleRange) {
            return LoadSingleRangeLumpsImpl(range, lumpIds);
        } else if (WadType == MultiRange) {
            return LoadMultiRangeLumpsImpl(range, lumpIds);
        } else {
            throw yexception() << "Unable to load range from empty loader";
        }
    }

    TVector<TBlob> LoadRangeLumps(ui32 range, const TConstArrayRef<TWadLumpId> lumpIds) const {
        TVector<TString> names;
        for (const auto& id: lumpIds) {
            names.push_back(ToString(id));
        }
        return LoadRangeLumps(range, names);
    }

    TBlob LoadRangeLump(ui32 range, const TString& lumpId) const {
        return LoadRangeLumps(range, {lumpId})[0];
    }

    TBlob LoadRangeLump(ui32 range, const TWadLumpId& lumpId) const {
        return LoadRangeLumps(range, {ToString(lumpId)})[0];
    }

    static ERangesWadType ParseWadType(const THolder<NDoom::IWad>& wad) {
        if (wad == nullptr)
            return Unknown;
        return wad->HasGlobalLump(RangesProtoId) ? MultiRange : SingleRange;
    }

    bool Empty() const {
        return NumRanges() == 0;
    }

    void Clear() {
        WadType = Unknown;
        RangeWadMapping_.clear();
        GlobalWadRanges_.clear();
    }

    ERangesWadType GetType() const {
        return WadType;
    }

    size_t NumRanges() const {
        return RangeWadMapping_.size();
    }

private:
    void EnsureWadType(const THolder<NDoom::IWad>& wad) {
        if (WadType == Unknown) {
            WadType = ParseWadType(wad);
        } else {
            ERangesWadType type = ParseWadType(wad);
            Y_ENSURE(WadType == type || type == Unknown);
        }
    }

    TVector<TBlob> LoadSingleRangeLumpsImpl(ui32 range, const TConstArrayRef<TString> lumpIds) const {
        Y_ENSURE(WadType == SingleRange);
        const size_t *id = RangeWadMapping_.FindPtr(range);
        if (id == nullptr) {
            RangeDoesNotExist(range);
            return TVector<TBlob>(lumpIds.size());
        }
        TVector<TBlob> res;
        for (auto const& lumpId: lumpIds) {
            res.push_back(GlobalWadRanges_[*id]->LoadGlobalLump(lumpId));
        }
        return res;
    }

    TVector<TBlob> LoadMultiRangeLumpsImpl(ui32 range, const TConstArrayRef<TString> lumpIds) const {
        Y_ENSURE(WadType == MultiRange);
        const size_t *id = RangeWadMapping_.FindPtr(range);
        if (id == nullptr) {
            RangeDoesNotExist(range);
            return TVector<TBlob>(lumpIds.size());
        }
        auto &wadPtr = GlobalWadRanges_[*id];
        TVector<size_t> mapping(lumpIds.size());
        wadPtr->MapDocLumps(lumpIds, mapping);
        TVector<TArrayRef<const char>> regions(lumpIds.size());
        TBlob docBlob = wadPtr->LoadDocLumps(range, mapping, regions);

        TVector<TBlob> res;
        const char *start = docBlob.AsCharPtr();
        for (const auto& region: regions) {
            auto regionStart = region.data() - start;
            res.push_back(docBlob.SubBlob(regionStart, regionStart + region.size()));
        }
        return res;
    }

    void RangeDoesNotExist(ui32 range) const {
        SEARCH_ERROR << "Range " << range << " not found in global wad ranges for index type " << IndexType;
    }

    ERangesWadType WadType = Unknown;
    THashMap<ui32 /*range*/, size_t /*index in GlobalWadRanges_*/> RangeWadMapping_;  // Used only for MultiRange wads
    TVector<THolder<NDoom::IWad>> GlobalWadRanges_;
};

struct TRangedKeyId {
    ui32 Id = 0;
    ui32 Range = 0;
};

inline bool operator==(TRangedKeyId lhs, TRangedKeyId rhs) {
    return std::tie(lhs.Id, lhs.Range) == std::tie(rhs.Id, rhs.Range);
}

struct TOffroadWadKey {
    TRangedKeyId Id;
    TString PackedKey;

    TOffroadWadKey() = default;

    TOffroadWadKey(TRangedKeyId id, const TString& packedKey)
        : Id(id)
        , PackedKey(packedKey)
    {

    }

    friend bool operator==(const TOffroadWadKey& l, const TOffroadWadKey& r) {
        return std::tie(l.Id, l.PackedKey) == std::tie(r.Id, r.PackedKey);
    }
};

template <typename Hit>
class IOffroadWadIteratorFactory {
public:
    using IIterator = IOffroadWadIterator<Hit>;
    virtual THolder<IIterator> MakeIterator() const = 0;
};


/* Depends only on the global part of the index and can be used without the document part.
 * Useful when the index is split into independent global and per-document parts.
 */
template <typename Hit>
class IOffroadWadKeySearcher {
public:
    using IIterator = IOffroadWadIterator<Hit>;
    using TKeyId = TRangedKeyId;
    using THit = Hit;

    virtual ~IOffroadWadKeySearcher() = default;

    virtual bool FindTerms(TStringBuf lemma, IIterator* iterator, std::vector<TOffroadWadKey>* keys) = 0;
};

/* Same as IOffroadWadKeySearcher, depends only on one part of a split index.
 */
template <typename Hit>
class IOffroadWadHitSearcher {
public:
    using IIterator = IOffroadWadIterator<Hit>;
    using TKeyId = TRangedKeyId;
    using THit = Hit;

    virtual ~IOffroadWadHitSearcher() = default;

    virtual void PreLoadDoc(ui32 docId, const NDoom::TSearchDocLoader& loader, IIterator* iterator) = 0;
    virtual bool Find(ui32 docId, TKeyId termId, IIterator* iterator) = 0;
    virtual void FetchDocs(TConstArrayRef<ui32> docIds, std::function<void(ui32)>&& consumer, IIterator* iterator) = 0;
    virtual void PrefetchDocs(const TVector<ui32>& docIds, IIterator* iterator) = 0;
    virtual size_t GetDocCount() const = 0;
};

template <class Hit>
class IOffroadWadSearcher:
    public IOffroadWadKeySearcher<Hit>,
    public IOffroadWadHitSearcher<Hit>,
    public IOffroadWadIteratorFactory<Hit>
{
    static_assert(std::is_same_v<typename IOffroadWadKeySearcher<Hit>::IIterator, typename IOffroadWadHitSearcher<Hit>::IIterator>, "");

public:
    using typename IOffroadWadKeySearcher<Hit>::IIterator;
    using typename IOffroadWadKeySearcher<Hit>::TKeyId;
    using typename IOffroadWadHitSearcher<Hit>::THit;
};

template <EWadIndexType IndexType, class Hit, class Vectorizer, class Subtractor, class PrefixVectorizer>
class TOffroadWadKeySearcher final: public IOffroadWadKeySearcher<Hit>, public IOffroadWadIteratorFactory<Hit> {
    using TKeySeeker = NOffroad::TFatKeySeeker<ui32, NOffroad::TNullSerializer>;
public:
    using TBase = IOffroadWadKeySearcher<Hit>;

    using typename TBase::IIterator;
    using typename TBase::TKeyId;
    using TKey = TString;
    using TKeyRef = TStringBuf;
    using TIterator = TOffroadWadIterator<Hit, Vectorizer, Subtractor, PrefixVectorizer>;

    using TKeyTable = typename TIterator::TKeyReader::TTable;
    using TKeyModel = typename TIterator::TKeyReader::TModel;

    TOffroadWadKeySearcher() = default;

    TOffroadWadKeySearcher(NDoom::TKeyInvPortionsKeyRanges&& keyRanges, TOffroadWadRangesLoader<IndexType>&& globalWadRangesLoader)
    {
        Reset(std::move(keyRanges), std::move(globalWadRangesLoader));
    }

    // Doesn't transfer ownership, wad must be kept alive outside until it's no longer needed
    TOffroadWadKeySearcher(NDoom::IWad* wad)
    {
        Reset(wad);
    }

    void Reset(NDoom::TKeyInvPortionsKeyRanges&& keyRanges, TOffroadWadRangesLoader<IndexType>&& globalWadRangesLoader) {
        Clear();
        KeyRanges_ = std::move(keyRanges);
        GlobalWadRangesLoader_ = std::move(globalWadRangesLoader);
        Init();
    }

    // Doesn't transfer ownership, wad must be kept alive outside until it's no longer needed
    void Reset(NDoom::IWad* wad) {
        Y_ENSURE(wad != nullptr);
        Clear();
        Wad_ = wad;
        Init();
    }

    THolder<IOffroadWadIterator<Hit>> MakeIterator() const override {
        return MakeHolder<TIterator>();
    }

    bool FindTerms(TStringBuf lemma, IIterator* baseIterator, std::vector<TOffroadWadKey>* keys) final {
        TIterator* iterator = static_cast<TIterator*>(baseIterator);
        ui32 lemmaRange = 0;
        if (Y_LIKELY(!lemma.empty())) {
            lemmaRange = KeyRanges_.Range(lemma);
        }

        PwnIterator(iterator, lemmaRange);

        ui32 localData;
        TStringBuf localKey;
        if (!KeySeekers_[lemmaRange].LowerBound(lemma, &localKey, &localData, &iterator->KeyReader_))
            return false;

        bool result = false;
        while (iterator->KeyReader_.ReadKey(&localKey, &localData)) {
            if (!localKey.StartsWith(lemma))
                return result;
            if (localKey.size() > lemma.size() && (ui8)localKey[lemma.size()] > INFO_BYTE_MAX_VALUE)
                return result;

            TOffroadWadKey& key = keys->emplace_back();
            key.PackedKey.assign(localKey);
            key.Id.Id = localData;
            key.Id.Range = lemmaRange;

            result = true;
        }

        return result;
    }

private:
    void Init() {
        Y_ASSERT(Wad_ != nullptr || !GlobalWadRangesLoader_.Empty());

        const ui32 numRanges = KeyRanges_.NumRanges();
        const ui32 foundRanges = GlobalWadRangesLoader_.NumRanges();
        Y_ENSURE(foundRanges <= numRanges || (foundRanges == 0 && numRanges == 1));
        if (numRanges > 1 && foundRanges < numRanges) {
            SEARCH_WARNING << "Found only " << foundRanges << " out of " << numRanges;
        }

        TKeyModel keyModel;
        keyModel.Load(LoadLump(0, TWadLumpId(IndexType, EWadLumpRole::KeysModel)));
        KeyTable_.Reset(keyModel);

        KeysBlobs_.resize(numRanges);
        KeySeekers_.resize(numRanges);
        for (ui32 i = 0; i < numRanges; ++i) {
            TVector<TBlob> blobs = LoadLumps(i, {
                TWadLumpId(IndexType, EWadLumpRole::Keys),
                TWadLumpId(IndexType, EWadLumpRole::KeyFat),
                TWadLumpId(IndexType, EWadLumpRole::KeyIdx)
            });
            KeysBlobs_[i] = std::move(blobs[0]);
            KeySeekers_[i].Reset(std::move(blobs[1]), std::move(blobs[2]));
        }
    }
    void Clear() {
        GlobalWadRangesLoader_.Clear();
        KeyRanges_ = {{ "" }};
        Wad_ = nullptr;
        KeyTable_ = TKeyTable();
        KeysBlobs_.clear();
        KeySeekers_.clear();
    }

    TBlob LoadLump(ui32 range, const NDoom::TWadLumpId& id) {
        if (!GlobalWadRangesLoader_.Empty()) {
            return GlobalWadRangesLoader_.LoadRangeLump(range, id);
        } else {
            Y_ENSURE(range == 0);
            return Wad_->LoadGlobalLump(id);
        }
    }

    TVector<TBlob> LoadLumps(ui32 range, const TVector<const NDoom::TWadLumpId>& ids) {
        if (!GlobalWadRangesLoader_.Empty()) {
            return GlobalWadRangesLoader_.LoadRangeLumps(range, ids);
        } else {
            Y_ENSURE(range == 0);
            TVector<TBlob> res;
            for (const auto& id: ids) {
                res.push_back(Wad_->LoadGlobalLump(id));
            }
            return res;
        }
    }

    void PwnIterator(TIterator* iterator, ui32 range) {
        if (iterator->Tag() != this || (iterator->KeyRange_ != range)) {
            iterator->KeyRange_ = range;
            iterator->KeyReader_.Reset(&KeyTable_, KeysBlobs_[range]);
        }

        if (iterator->Tag() != this) {
            iterator->SetTag(this);
            iterator->PrefetchedDocBlobs_.clear();
            iterator->PrefetchedDocIds_.clear();
            iterator->PrefetchedDocRegions_.clear();
        }
    }
private:
    NDoom::TKeyInvPortionsKeyRanges KeyRanges_ = {{ "" }};
    TOffroadWadRangesLoader<IndexType> GlobalWadRangesLoader_;
    NDoom::IWad* Wad_ = nullptr;
    TKeyTable KeyTable_;
    TVector<TBlob> KeysBlobs_;
    TVector<TKeySeeker> KeySeekers_;
};


template <EWadIndexType IndexType, class Hit, class Vectorizer, class Subtractor, class PrefixVectorizer>
class TOffroadWadHitSearcher final: public IOffroadWadHitSearcher<Hit>, public IOffroadWadIteratorFactory<Hit> {
private:
    using TKeySeeker = NOffroad::TFatKeySeeker<ui32, NOffroad::TNullSerializer>;

public:
    using TBase = IOffroadWadSearcher<Hit>;
    using IIterator = typename TBase::IIterator;
    using TKeyId = typename TBase::TKeyId;

    using THit = Hit;
    using TIterator = TOffroadWadIterator<Hit, Vectorizer, Subtractor, PrefixVectorizer>;

    using THitTable = typename TIterator::THitReader::TTable;
    using THitModel = typename TIterator::THitReader::TModel;

    struct THitTableFactory {
        static THolder<THitTable> Create(const TBlob& data) {
            THitModel hitModel;
            hitModel.Load(data);
            return MakeHolder<THitTable>(hitModel);
        }
    };

    static inline void EnableHitTablesDeduplication() {
        Deduplicator<THitTableFactory>().DeduplicationEnabled = true;
    }

    TOffroadWadHitSearcher() = default;

    TOffroadWadHitSearcher(IWad* wad)
    {
        Reset(wad);
    }

    TOffroadWadHitSearcher(IChunkedWad* wad)
    {
        Reset(wad);
    }

    TOffroadWadHitSearcher(IChunkedWad* wad, const NDoom::IDocLumpMapper* mapper)
    {
        Reset(wad, mapper);
    }

    void Reset(IWad* wad) {
        LocalWad_.Reset(new TSingleChunkedWad(wad));
        Reset(LocalWad_.Get());
    }

    void Reset(IChunkedWad* wad) {
        Reset(wad, wad);
    }

    void Reset(IChunkedWad* wad, const IDocLumpMapper* mapper) {
        Size_ = wad->Size();
        Wad_ = wad;

        std::array<TWadLumpId, 2> types = {{ TWadLumpId(IndexType, EWadLumpRole::Hits), TWadLumpId(IndexType, EWadLumpRole::HitSub) }};
        mapper->MapDocLumps(types, Mapping_);

        HitTables_.resize(Wad_->Chunks());
        for (size_t i = 0; i < Wad_->Chunks(); ++i) {
            HitTables_[i] = Deduplicator<THitTableFactory>().GetOrCreate(Wad_->LoadChunkGlobalLump(i, TWadLumpId(IndexType, EWadLumpRole::HitsModel)));
        }
    }

    THolder<IOffroadWadIterator<Hit>> MakeIterator() const final {
        return MakeHolder<TIterator>();
    }

    void PreLoadDoc(ui32 docId, const NDoom::TSearchDocLoader& loader, IIterator* baseIterator) final {
        TIterator* iterator = static_cast<TIterator*>(baseIterator);
        PwnIterator(iterator);

        iterator->PrefetchedDocIds_.resize(1);
        iterator->PrefetchedDocRegions_.resize(1);
        iterator->PrefetchedDocBlobs_.resize(1);

        iterator->PrefetchedDocBlobs_[0] = loader.DataHolder();
        loader.LoadDocRegions(Mapping_, iterator->PrefetchedDocRegions_[0]);
        iterator->PrefetchedDocIds_[0] = docId;
    }

    void FetchDocs(TConstArrayRef<ui32> docIds, std::function<void(ui32)>&& consumer, IIterator* baseIterator) final {
        TIterator* iterator = static_cast<TIterator*>(baseIterator);
        PwnIterator(iterator);
        iterator->PrefetchedDocIds_.resize(1);
        iterator->PrefetchedDocRegions_.resize(1);
        iterator->PrefetchedDocBlobs_.resize(1);

        Wad_->LoadDocLumps(docIds, Mapping_,
            [&](size_t i, TMaybe<NDoom::IWad::TDocLumpData>&& docLumpData) {
                if (docLumpData) {
                    Y_ASSERT(docLumpData->Regions.size() == iterator->PrefetchedDocRegions_[0].size());
                    for (size_t i = 0; i < iterator->PrefetchedDocRegions_[0].size(); ++i) {
                        iterator->PrefetchedDocRegions_[0][i] = docLumpData->Regions[i];
                    }
                    iterator->PrefetchedDocBlobs_[0] = std::move(docLumpData->Blob);
                    iterator->PrefetchedDocIds_[0] = docIds[i];

                    consumer(docIds[i]);
                }
            });
    }

    void PrefetchDocs(const TVector<ui32>& docIds, IIterator* baseIterator) final {
        TIterator* iterator = static_cast<TIterator*>(baseIterator);
        Y_ENSURE(IsSorted(docIds.begin(), docIds.end()));

        PwnIterator(iterator);

        if (iterator->PrefetchedDocIds_.size() == docIds.size()) {
            bool diff = false;
            for (size_t i = 0; i < docIds.size(); ++i) {
                if (iterator->PrefetchedDocIds_[i] != docIds[i]) {
                    diff = true;
                    break;
                }
            }
            if (!diff) {
                return;
            }
        }

        iterator->PrefetchedDocIds_.assign(docIds.begin(), docIds.end());

        iterator->PrefetchedDocRegions_.resize(docIds.size());

        if (docIds.size() <= 16) {
            std::array<TArrayRef<TArrayRef<const char>>, 16> view;
            for (size_t i = 0; i < docIds.size(); ++i) {
                view[i] = TArrayRef<TArrayRef<const char>>(iterator->PrefetchedDocRegions_[i]);
            }

            iterator->PrefetchedDocBlobs_ = Wad_->LoadDocLumps(
                iterator->PrefetchedDocIds_,
                Mapping_,
                TArrayRef<TArrayRef<TArrayRef<const char>>>(view.data(), docIds.size()));
        } else {
            TVector<TArrayRef<TArrayRef<const char>>> view(docIds.size());
            for (size_t i = 0; i < docIds.size(); ++i) {
                view[i] = TArrayRef<TArrayRef<const char>>(iterator->PrefetchedDocRegions_[i]);
            }

            iterator->PrefetchedDocBlobs_ = Wad_->LoadDocLumps(
                iterator->PrefetchedDocIds_,
                Mapping_,
                view);
        }
    }

    bool Find(ui32 docId, TKeyId termId, IIterator* baseIterator) final {
        TIterator* iterator = static_cast<TIterator*>(baseIterator);
        if (docId >= Size_)
            return false;

        PwnIterator(iterator);
        PositionIterator(iterator, docId);

        THit prefix;
        prefix.SetDocId(termId.Id);
        prefix.SetRange(termId.Range);

        THit first;
        if (!iterator->HitReader_.LowerBound(prefix, &first))
            return false;

        if (first.DocId() != termId.Id || first.Range() != termId.Range)
            return false;

        iterator->TermId_ = termId.Id;
        iterator->TermRange_ = termId.Range;
        return true;
    }

    size_t GetDocCount() const final {
        return Size_;
    }

private:
    void PwnIterator(TIterator* iterator) {
        if (iterator->Tag() != this) {
            iterator->SetTag(this);
            iterator->PrefetchedDocBlobs_.clear();
            iterator->PrefetchedDocIds_.clear();
            iterator->PrefetchedDocRegions_.clear();
        }
    }

    void PositionIterator(TIterator* iterator, ui32 docId) {
        if (iterator->DocId_ != docId) {
            iterator->DocId_ = docId;

            if (!iterator->PrefetchedDocIds_.empty()) {
                const size_t index = LowerBound(iterator->PrefetchedDocIds_.begin(), iterator->PrefetchedDocIds_.end(), docId) - iterator->PrefetchedDocIds_.begin();
                if (index != iterator->PrefetchedDocIds_.size() && iterator->PrefetchedDocIds_[index] == docId) {
                    iterator->Blob_ = iterator->PrefetchedDocBlobs_[index];
                    iterator->HitReader_.Reset(HitTables_[Wad_->DocChunk(docId)].Get(), iterator->PrefetchedDocRegions_[index][0], iterator->PrefetchedDocRegions_[index][1]);
                    return;
                }
            }

            std::array<TArrayRef<const char>, 2> regions;
            iterator->Blob_ = Wad_->LoadDocLumps(docId, Mapping_, regions);
            iterator->HitReader_.Reset(HitTables_[Wad_->DocChunk(docId)].Get(), regions[0], regions[1]);
        }
    }

private:
    THolder<IChunkedWad> LocalWad_;
    TVector<TAtomicSharedPtr<const THitTable>> HitTables_;
    std::array<size_t, 2> Mapping_;
    IChunkedWad* Wad_ = nullptr;
    size_t Size_ = 0;
};


// Used to create an instance of TOffroadWadSearcher that doesn't depend on the global part of the index,
// effectively turning it into TOffroadWadHitSearcher. Used for backward compatibility, new code should use
// TOffroadWadHitSearcher directly.
struct TOffroadWadSearcherOnlyHitsTag {};

template <EWadIndexType IndexType, class Hit, class Vectorizer, class Subtractor, class PrefixVectorizer>
class TOffroadWadSearcher final: public IOffroadWadSearcher<Hit> {
private:
    using TKeySeeker = NOffroad::TFatKeySeeker<ui32, NOffroad::TNullSerializer>;

public:
    using TBase = IOffroadWadSearcher<Hit>;
    using IIterator = typename TBase::IIterator;
    using TKeyId = typename TBase::TKeyId;

    using THit = Hit;
    using TKey = TString;
    using TKeyRef = TStringBuf;
    using TIterator = TOffroadWadIterator<Hit, Vectorizer, Subtractor, PrefixVectorizer>;

    using THitTable = typename TIterator::THitReader::TTable;
    using TKeyTable = typename TIterator::TKeyReader::TTable;
    using THitModel = typename TIterator::THitReader::TModel;
    using TKeyModel = typename TIterator::TKeyReader::TModel;

    TOffroadWadSearcher(IWad* wad)
    {
        Reset(wad);
    }

    TOffroadWadSearcher(IChunkedWad* wad, NDoom::TKeyInvPortionsKeyRanges&& keyRanges = {{ "" }}, TVector<std::pair<THolder<NDoom::IWad>, ui32>>&& globalWadRanges = {})
    {
        TOffroadWadRangesLoader<IndexType> loader(std::move(globalWadRanges));
        Reset(wad, std::move(keyRanges), std::move(loader));
    }

    TOffroadWadSearcher(IChunkedWad* wad, const NDoom::IDocLumpMapper* mapper, NDoom::TKeyInvPortionsKeyRanges&& keyRanges = {{ "" }}, TVector<std::pair<THolder<NDoom::IWad>, ui32>>&& globalWadRanges = {})
    {
        TOffroadWadRangesLoader<IndexType> loader(std::move(globalWadRanges));
        Reset(wad, mapper, std::move(keyRanges), std::move(loader));
    }

    TOffroadWadSearcher(IChunkedWad* wad, TOffroadWadSearcherOnlyHitsTag) {
        Reset(wad, TOffroadWadSearcherOnlyHitsTag{});
    }

    TOffroadWadSearcher(IChunkedWad* wad, const NDoom::IDocLumpMapper* mapper, TOffroadWadSearcherOnlyHitsTag) {
        Reset(wad, mapper, TOffroadWadSearcherOnlyHitsTag{});
    }

    void Reset(IWad* wad) {
        KeySearcher_.ConstructInPlace(wad);
        HitSearcher_.Reset(wad);
    }

    void Reset(IChunkedWad* wad, NDoom::TKeyInvPortionsKeyRanges&& ranges = {{ "" }}, TOffroadWadRangesLoader<IndexType>&& globalWadRangesLoader = {}) {
        Reset(wad, wad, std::move(ranges), std::move(globalWadRangesLoader));
    }

    void Reset(IChunkedWad* wad, TOffroadWadSearcherOnlyHitsTag) {
        Reset(wad, wad, TOffroadWadSearcherOnlyHitsTag{});
    }

    void Reset(IChunkedWad* wad, const IDocLumpMapper* mapper, TOffroadWadSearcherOnlyHitsTag) {
        KeySearcher_.Clear();
        HitSearcher_.Reset(wad, mapper);
    }

    void Reset(IChunkedWad* wad, const IDocLumpMapper* mapper, NDoom::TKeyInvPortionsKeyRanges&& ranges = {{ "" }}, TOffroadWadRangesLoader<IndexType>&& globalWadRangesLoader = {}) {
        const ui32 numRanges = ranges.NumRanges();
        const ui32 foundRanges = globalWadRangesLoader.NumRanges();
        Y_ENSURE(foundRanges <= numRanges || (foundRanges == 0 && numRanges == 1));
        if (numRanges > 1 && foundRanges < numRanges) {
            SEARCH_WARNING << "Found only " << foundRanges << " out of " << numRanges;
        }

        if (globalWadRangesLoader.Empty()) {
            KeySearcher_.ConstructInPlace(wad);
        } else {
            KeySearcher_.ConstructInPlace(std::move(ranges), std::move(globalWadRangesLoader));
        }

        HitSearcher_.Reset(wad, mapper);
    }

    THolder<IOffroadWadIterator<Hit>> MakeIterator() const final {
        return HitSearcher_.MakeIterator();
    }

    void PreLoadDoc(ui32 docId, const NDoom::TSearchDocLoader& loader, IIterator* baseIterator) final {
        HitSearcher_.PreLoadDoc(docId, loader, baseIterator);
    }

    bool FindTerms(TStringBuf lemma, IIterator* baseIterator, std::vector<TOffroadWadKey>* keys) final {
        // An exception thrown here means that the global part of the index, needed for KeySearcher_ to work
        // is turned off or doesn't exist at all. Probably, a configuration problem.
        // See SEARCH-10851
        Y_ENSURE(KeySearcher_.Defined(), "KeySearcher is not initialized");
        return KeySearcher_->FindTerms(lemma, baseIterator, keys);
    }

    void FetchDocs(TConstArrayRef<ui32> docIds, std::function<void(ui32)>&& consumer, IIterator* baseIterator) final {
        HitSearcher_.FetchDocs(docIds, std::move(consumer), baseIterator);
    }

    void PrefetchDocs(const TVector<ui32>& docIds, IIterator* baseIterator) final {
        HitSearcher_.PrefetchDocs(docIds, baseIterator);
    }

    bool Find(ui32 docId, TKeyId termId, IIterator* baseIterator) final {
        return HitSearcher_.Find(docId, termId, baseIterator);
    }

    size_t GetDocCount() const override {
        return HitSearcher_.GetDocCount();
    }

private:
    TMaybe<TOffroadWadKeySearcher<IndexType, Hit, Vectorizer, Subtractor, PrefixVectorizer>> KeySearcher_;
    TOffroadWadHitSearcher<IndexType, Hit, Vectorizer, Subtractor, PrefixVectorizer> HitSearcher_;
};
} // namespace NDoom
