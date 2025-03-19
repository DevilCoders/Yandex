#pragma once

#include <util/generic/noncopyable.h>
#include <util/generic/string.h>

#include <kernel/keyinv/indexfile/oldindexfile.h>
#include <kernel/keyinv/indexfile/indexreader.h>
#include <kernel/keyinv/indexfile/fat.h>
#include <kernel/keyinv/hitlist/positerator.h>    /* For TPosIterator. */

#include <kernel/doom/progress/progress.h>

#include "yandex_key_data.h"

namespace NDoom {


/**
 * `TSequentialYandexReader` is a common yndex-format invindex reader.
 * It is faster than `TYandexReader` for sequential access because it uses
 * a sequential key-blocks reader.
 *
 * However it doesn't support random seeks :)
 */
template<class Hit>
class TSequentialYandexReader: private TNonCopyable {
public:
    using THit = Hit;
    using TKey = TString;
    using TKeyRef = TStringBuf;
    using TKeyData = TYandexKeyData;

    enum {
        HasLowerBound = false
    };

    TSequentialYandexReader() = default;

    explicit TSequentialYandexReader(const TString& prefix, bool useInvMmap = false) {
        Reset(prefix, useInvMmap);
    }

    void Reset(const TString& prefix, bool useInvMmap = false) {
        IndexPrefix_ = prefix;
        UseInvMmap_ = useInvMmap;
        Restart();
    }

    void Restart() {
        IndexFile_.Reset(new NIndexerCore::TInputIndexFile(IndexPrefix_.data(), IYndexStorage::FINAL_FORMAT));
        InvKeyReader_.Reset(new NIndexerCore::TInvKeyReader(*IndexFile_));
        InvMap_.Reset(new TMemoryMap(IndexFile_->CreateInvMapping()));
        IsIteratorInitialized_ = false;

        /* We get total key count from FAT. Slow, but works. */
        if (Size_ == 0) {
            TFastAccessTable fat;
            fat.Open(*IndexFile_);
            Size_ = fat.KeyCount();
        }
        Position_ = 0;
    }

    bool ReadKey(TKeyRef* key, TKeyData* data = NULL) {
        IsIteratorInitialized_ = false;
        if (!InvKeyReader_->ReadNext())
            return false;

        *key = InvKeyReader_->GetKeyText();
        if(data)
            data->SetHitCount(InvKeyReader_->GetCount());

        Position_++;
        return true;
    }

    bool ReadHit(THit* hit) {
        if (!IsIteratorInitialized_) {
            if (UseInvMmap_) {
                InvKeyReader_->InitPosIterator(*InvMap_, Iterator_);
            } else {
                TFile tempFile = InvMap_->GetFile();
                Y_ASSERT(tempFile.IsOpen());
                InvKeyReader_->InitPosIterator(tempFile, Iterator_);
            }
            IsIteratorInitialized_ = true;
        }
        if (!Iterator_.Valid()) {
            return false;
        }
        CopyHitInternal(Iterator_.Current(), hit);
        Iterator_.Next();
        return true;
    }

    void LowerBound(const TKeyRef&) {
        Y_VERIFY(false, "LowerBound unsupported, check HasLowerBound property.");
    }

    TProgress Progress() const {
        return TProgress(Position_, Size_);
    }

private:
    static void CopyHitInternal(const SUPERLONG& src, SUPERLONG* dst) {
        *dst = src;
    }

    template<class OtherHit>
    static void CopyHitInternal(const SUPERLONG& src, OtherHit* dst) {
        *dst = OtherHit::FromSuperLong(src);
    }

private:
    TString IndexPrefix_;
    bool UseInvMmap_ = false;

    THolder<NIndexerCore::TInputIndexFile> IndexFile_;
    THolder<NIndexerCore::TInvKeyReader> InvKeyReader_;
    TPosIterator<> Iterator_;
    bool IsIteratorInitialized_ = false;

    ui64 Position_ = 0;
    ui64 Size_ = 0;

    THolder<TMemoryMap> InvMap_;
};

} // namespace NDoom
