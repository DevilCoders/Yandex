#pragma once

#include <util/generic/noncopyable.h>
#include <util/generic/string.h>

#include <kernel/keyinv/indexfile/rdkeyit.h>
#include <kernel/keyinv/indexfile/indexreader.h>

#include <kernel/doom/progress/progress.h>

namespace NDoom {

/**
 * TUnsortedYandexReader<Hit> is for reading m2nsort-produced partially sorted
 * indices (more precisely, that were merged without sort).
 *
 * It doesn't allow to seek for the keys in random order.
 */
template<class Hit>
class TUnsortedYandexReader: private TNonCopyable {
public:
    using THit = Hit;
    using TKey = TString;
    using TKeyRef = TStringBuf;
    using TKeyData = void;
private:
    /**
     * Unsorted hits iterator
     **/
    class TUnsortedBufferedHitIterator: public TBufferedHitIterator {
    private:
        ui32 HitsPos = 0;
        ui32 Count = 0;
    public:
        inline void Restart(SUPERLONG start, ui32 length, ui32 count, bool memorize = false) {
            TBufferedHitIterator::Restart(start, length, count, memorize);
            Count = count;
            HitsPos = UNSORTED_HIT_BUFFER_SIZE;
        }

        inline bool Next() {
            if (HitsPos == 0) {
                Count -= UNSORTED_HIT_BUFFER_SIZE;
                if (Count == 0) {
                    return false;
                }
                HitDecoder.Reset();
                HitDecoder.SetSize(Count);
                HitDecoder.ReadHeader(Cur);
                if (Count < 8) {
                    HitDecoder.FallBackDecode(Cur, Count);
                }
                HitsPos = UNSORTED_HIT_BUFFER_SIZE;
            }
            HitsPos--;
            return TBufferedHitIterator::Next();
        }
    }; // class TUnsortedBufferedHitIterator

public:
    enum {
        HasLowerBound = false
    };

    explicit TUnsortedYandexReader(const TString& prefix, bool useInvMmap = false)
        : IndexPrefix_(prefix)
        , UseInvMmap_(useInvMmap)
    {
        Restart();
    }

    void Restart() {
        IndexFile_.Reset(new NIndexerCore::TInputIndexFile(IYndexStorage::FINAL_FORMAT, YNDEX_VERSION_FINAL_DEFAULT));
        IndexFile_->Open((IndexPrefix_ + "key").data(), /*inv=*/ nullptr);

        InvKeyReader_.Reset(new NIndexerCore::TInvKeyReader(*IndexFile_));
        //InvMap_.Reset(new TMemoryMap(IndexFile_->CreateInvMapping()));

        InvFile_.Open((IndexPrefix_ + "inv").data(), 0x400000);

        Hits_.Init(InvFile_);
        IsIteratorInitialized_ = false;
    }

    bool ReadKey(TKeyRef* key, TKeyData* = nullptr) {
        IsIteratorInitialized_ = false;
        if (!InvKeyReader_->ReadNext()) {
            NextChunk();
            return false;
        }
        *key = InvKeyReader_->GetKeyText();
        return true;
    }

    bool ReadHit(THit* hit) {
        if (!IsIteratorInitialized_) {
            NextChunk();
        }

        if (!Hits_.Valid()) {
            return false;
        }
        CopyHitInternal(Hits_.Current(), hit);
        Hits_.Next();
        return true;
    }

    ui32 HitsCount() const {
        return InvKeyReader_->GetCount();
    }

    void LowerBound(const TKeyRef&) {
        Y_VERIFY(false, "LowerBound unsupported, check HasLowerBound property.");
    }

    TProgress Progress() const {
        // FIXME(mvel): report real progress
        return TProgress();
    }

private:
    void NextChunk() { // aka ReadHit() in m2nsort
#if 0
        if (UseInvMmap_) {
            InvKeyReader_->InitPosIterator(*InvMap_, Iterator_);
        } else {
            TFile tempFile = InvMap_->GetFile();
            Y_ASSERT(tempFile.IsOpen());
            InvKeyReader_->InitPosIterator(tempFile, Iterator_);
        }
#endif
        Hits_.Restart(
            InvKeyReader_->GetOffset(),
            GetHitsLength<HIT_FMT_BLK8>(InvKeyReader_->GetLength(), HitsCount(), HIT_FMT_BLK8),
            HitsCount()
        );
        IsIteratorInitialized_ = true;
    }

    static void CopyHitInternal(const SUPERLONG& src, SUPERLONG* dst) {
        *dst = src;
    }

    template<class OtherHit>
    static void CopyHitInternal(const SUPERLONG& src, OtherHit* dst) {
        *dst = OtherHit::FromSuperLong(src);
    }

private:
    const TString IndexPrefix_;
    const bool UseInvMmap_ = false;

    THolder<NIndexerCore::TInputIndexFile> IndexFile_;
    THolder<NIndexerCore::TInvKeyReader> InvKeyReader_;
    NIndexerCore::TInputFile InvFile_;

    TUnsortedBufferedHitIterator Hits_;
    bool IsIteratorInitialized_ = false;

    //THolder<TMemoryMap> InvMap_; // FIXME(mvel) mapping is not used for now
};


} // namespace NDoom
