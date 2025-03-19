#pragma once

#include <util/generic/noncopyable.h>

#include <kernel/keyinv/indexfile/indexwriter.h>

namespace NDoom {

template<class Hit>
class TYandexWriter: public TNonCopyable {
public:
    using TKey = TString;
    using TKeyRef = TStringBuf;
    using THit = Hit;

    TYandexWriter() = default;

    TYandexWriter(const TString& path, ui32 targetVersion = YNDEX_VERSION_FINAL_DEFAULT) {
        Reset(path, targetVersion);
    }

    ~TYandexWriter() {
        Finish();
    }

    void Reset(const TString& path, ui32 targetVersion = YNDEX_VERSION_FINAL_DEFAULT) {
        Path_ = path;
        File_.Reset(new NIndexerCore::TOutputIndexFile((path + "key").data(), (path + "inv").data(), IYndexStorage::FINAL_FORMAT, targetVersion));
        Writer_.Reset(new NIndexerCore::TInvKeyWriter(*File_));
    }

    void WriteHit(const THit& hit) {
        WriteHitInternal(hit);
    }

    void WriteKey(const TKeyRef& key) {
        Writer_->WriteKey(key.data());
    }

    void WriteLayer() {

    }

    void Finish() {
        if (IsFinished())
            return;

        File_->CloseEx();
    }

    bool IsFinished() const {
        if (!File_)
            return true; /* We can get here if exception is thrown in TOutputIndexFile ctor. */

        return !File_->IsOpen();
    }

private:
    void WriteHitInternal(const SUPERLONG& hit) {
        Writer_->WriteHit(hit);
    }

    template<class OtherHit>
    void WriteHitInternal(const OtherHit& hit) {
        Writer_->WriteHit(hit.ToSuperLong());
    }

private:
    TString Path_;
    THolder<NIndexerCore::TOutputIndexFile> File_;
    THolder<NIndexerCore::TInvKeyWriter> Writer_;
};


} // namespace NDoom
