#pragma once

#include <kernel/doom/offroad_sent_wad/offroad_sent_wad_io.h>
#include <kernel/doom/search_fetcher/search_fetcher.h>
#include <kernel/sent_lens/data/sentence_lengths_coder_data.h>

#include <library/cpp/offroad/custom/subtractors.h>

#include <util/memory/blob.h>
#include <util/generic/vector.h>
#include <util/string/vector.h>
#include <util/generic/ptr.h>

using TSentenceOffsets = TPODVector<ui32>;

class ISentenceLengthsPreLoader {
public:
    virtual void AnnounceDocIds(TConstArrayRef<ui32> docIds) = 0;
    virtual void PreLoadDoc(ui32 docId, const NDoom::TSearchDocLoader& loader) = 0;
    virtual ~ISentenceLengthsPreLoader() = default;
};

// base class for realtime and big robot parts
class ISentenceLengthsReader {
public:
    virtual THolder<ISentenceLengthsPreLoader> CreatePreLoader() const = 0;
    virtual void GetOffsets(ui32 docId, TSentenceOffsets* result, ISentenceLengthsPreLoader* preLoader = nullptr) const = 0;
    virtual ~ISentenceLengthsReader() = default;
};

class ISentenceLengthsLenReader : public ISentenceLengthsReader {
public:
    virtual void Get(ui32 docId, TSentenceLengths* result, ISentenceLengthsPreLoader* preLoader = nullptr) const = 0;
    void GetOffsets(ui32 docId, TSentenceOffsets* result, ISentenceLengthsPreLoader* preLoader = nullptr) const override = 0;
    ~ISentenceLengthsLenReader() override = default;
};

class ISentenceLengthsWadReader {
public:
    virtual THolder<ISentenceLengthsPreLoader> CreatePreLoader() const = 0;
    virtual void GetOffsets(ui32 docId, TSentenceOffsets* result, ISentenceLengthsPreLoader* preLoader) const = 0;
    virtual void GetLengths(ui32 docId, TSentenceLengths* result, ISentenceLengthsPreLoader* preLoader) const = 0;
    virtual size_t Size() const = 0;
    virtual ~ISentenceLengthsWadReader() = default;
};

namespace NDoom {
    class IWad;
}

inline void CalcOffsetsFromLentgthsIntegrating(const TSentenceLengths& lengths, TSentenceOffsets* offsets) {
    offsets->resize(1);
    offsets->Reserve(+lengths + 1);
    for (auto len: lengths)
        offsets->push_back(len);
    NOffroad::Integrate1(*offsets);
}

template <class Io>
class TSentenceLengthsWadPreLoader final: public ISentenceLengthsPreLoader {
    using TSearcher = typename Io::TSearcher;
    using TIterator = typename TSearcher::TIterator;

public:
    void PreLoadDoc(ui32 docId, const NDoom::TSearchDocLoader& loader) override {
        if (UsePreloader_) {
            Searcher_->PreloadDoc(docId, &loader, &Iterator_);
        }
    }

    void AnnounceDocIds(TConstArrayRef<ui32> ids) override {
        if (UsePreloader_) {
            Searcher_->AnnounceDocIds(ids, &Iterator_);
        }
    }

private:
    template <typename Io2>
    friend class TSentenceLengthsWadReader;

    TSentenceLengthsWadPreLoader(TSearcher* searcher, bool usePreloader)
        : Searcher_{ searcher }
        , UsePreloader_{ usePreloader }
    {}

private:
    TSearcher* Searcher_ = nullptr;
    TIterator Iterator_;
    bool UsePreloader_ = false;
};

template <class Io>
class TSentenceLengthsWadReader: public ISentenceLengthsWadReader {
    using TSearcher = typename Io::TSearcher;
    using TIterator = typename TSearcher::TIterator;
public:
    explicit TSentenceLengthsWadReader(const NDoom::IWad* wad, const NDoom::IDocLumpMapper* mapper, bool usePreloader)
        : Searcher_(new TSearcher(wad, mapper))
        , UsePreloader_(usePreloader)
    {
    }

    THolder<ISentenceLengthsPreLoader> CreatePreLoader() const override {
        return THolder<ISentenceLengthsPreLoader>(new TSentenceLengthsWadPreLoader<Io>(Searcher_.Get(), UsePreloader_));
    }

    void GetOffsets(ui32 docId, TSentenceOffsets* result, ISentenceLengthsPreLoader* preLoader) const override {
        TSentenceLengths lengths;
        GetLengths(docId, &lengths, preLoader);
        CalcOffsetsFromLentgthsIntegrating(lengths, result);
    }

    void GetLengths(ui32 docId, TSentenceLengths* result, ISentenceLengthsPreLoader* preLoader) const override {
        TIterator* iterator = nullptr;
        if (UsePreloader_ && preLoader) {
            iterator = &static_cast<TSentenceLengthsWadPreLoader<Io>*>(preLoader)->Iterator_;
        }

        TMaybe<TIterator> localIterator;
        if (!iterator) {
            localIterator.ConstructInPlace();
            iterator = localIterator.Get();
        }

        result->clear();
        auto consumer = [&](const auto& hit) {
            result->push_back(hit.Length());
            return true;
        };

        if (Searcher_->Find(docId, iterator)) {
            while (iterator->ReadHits(consumer)) {}
        }
    }

    size_t Size() const override {
        return Searcher_->Size();
    }


private:
    THolder<TSearcher> Searcher_;
    bool UsePreloader_ = false;
};


class TSentenceLengthsReader final : public ISentenceLengthsLenReader {
    friend class TSentenceLengthsManager;
public:
    struct TUseWad{};

    THolder<ISentenceLengthsPreLoader> CreatePreLoader() const override;
    void Get(ui32 docId, TSentenceLengths* result, ISentenceLengthsPreLoader* preLoader = nullptr) const override;
    void GetOffsets(ui32 docId, TSentenceOffsets* result, ISentenceLengthsPreLoader* preLoader = nullptr) const override;
    ui32 GetSize() const;
    static bool HasUnderlyingWad(const TString& fileName);

    TSentenceLengthsReader(TUseWad, const TString& fileName, NDoom::EWadIndexType indexType, bool lockMem = false);
    TSentenceLengthsReader(const NDoom::IWad* wad, const NDoom::IDocLumpMapper* mapper, NDoom::EWadIndexType indexType);

    explicit TSentenceLengthsReader(const TString& fileName, bool lockMem = false);
    explicit TSentenceLengthsReader(const TBlob& blob, bool lockMem = false);
    explicit TSentenceLengthsReader(const TMemoryMap& mapping, bool lockMem = false);
    ~TSentenceLengthsReader() override;

    // used in robots
    static void GetPackedVersion2(const ui8*& begin, size_t& size);
    void GetPacked(ui32 docId, TSentenceLengths* result) const;
    ui32 GetVersion() const;

public:

    static const ui32 SL_VERSION = 2;
    static const size_t SIZEOF_HEADER = 1024;

#pragma pack(push, 1)
    struct TOffset {
        ui64 Offset;

        TOffset() = default;

        TOffset(ui64 offset)
            : Offset(offset)
        {
        }

        bool operator!=(const TOffset& rhs) const {
            return Offset != rhs.Offset;
        }

        bool operator==(const TOffset& rhs) const {
            return Offset == rhs.Offset;
        }
    };
#pragma pack(pop)

    static const TOffset INVALID_OFFSET;

private:
    void InternalInit();
    void InitFromWad(const NDoom::IWad* wad, const NDoom::IDocLumpMapper* mapper, NDoom::EWadIndexType indexType, bool usePreloader);
    template<typename T>
    void Get_(ui32 docId, T& integrator) const;

private:
    ui32 Version = 0;
    TBlob Data;
    const bool LockMem = false;
    const char* Packed = 0;
    const char* PackedEnd = 0;
    const TOffset* Offsets = nullptr;
    ui32 Size = 0;
    THolder<NDoom::IWad> LocalWad;
    THolder<ISentenceLengthsWadReader> WadSentReader;
    TString FileName;
};


class TRealtimeSentenceLengthReader final : public ISentenceLengthsLenReader {
private:
    typedef TVector<TString> TContainer;
    TContainer Container;

public:
    TRealtimeSentenceLengthReader(size_t maxDocs);

    virtual THolder<ISentenceLengthsPreLoader> CreatePreLoader() const override;
    void Get(ui32 docId, TSentenceLengths* result, ISentenceLengthsPreLoader* preLoader = nullptr) const override;
    void GetOffsets(ui32 docId, TSentenceOffsets* result, ISentenceLengthsPreLoader* preLoader = nullptr) const override;

    void SetSentencesLength(ui32 docId, const TString& sentencesLength);
};
