#pragma once

#include <ysite/yandex/erf/erf_manager.h>
#include <type_traits>

namespace NDoom {

template <class ErfIo>
class TErfReader: private TNonCopyable {
public:
    using TKey = TString;
    using TKeyRef = TStringBuf;
    using THit = typename ErfIo::TReader::THit;
    using TErfType = typename std::remove_pointer<THit>::type;

    TErfReader(const TString& path)
        : ErfManager_(path, false, /*strictMode*/ true)
        , Size_(ErfManager_.GetErfCount())
    {
        Restart();
    }

    void Restart() {
        CurDoc_ = 0;
    }

    bool ReadHit(THit* hit) {
        if (!HasHit_)
            return false;
        *hit = &ErfManager_.GetDocErf(CurDoc_ - 1, nullptr);
        HasHit_ = false;
        return true;
    }

     bool ReadDoc(ui32* docId) {
        if (!Advance())
            return false;
        HasHit_ = true;
        *docId = CurDoc_ - 1;
        return true;
    }

    TProgress Progress() const {
        return TProgress(CurDoc_, Size_);
    }

private:
    bool Advance() {
        CurDoc_++;
        return CurDoc_ != Size_ + 1;
    }

private:
    TDocErfManager<typename std::remove_cv<TErfType>::type> ErfManager_;
    bool HasHit_ = false;
    ui32 CurDoc_ = 0;
    ui32 Size_ = 0;
};

class THostErfReader: private TNonCopyable {
public:
    using TKey = TString;
    using TKeyRef = TStringBuf;
    using THit = const THostErfInfo*;
    using TErfType = THostErfInfo;

    THostErfReader(const TString& path)
        : ErfManager_(path, false, /*strictMode*/ true, /*wad*/ false, /*lockMemory*/ false)
        , Size_(ErfManager_.GetHerf().GetCapacity())
    {
        Restart();
    }

    void Restart() {
        CurDoc_ = 0;
    }

    bool ReadHit(THit* hit) {
        if (!HasHit_)
            return false;
        *hit = &ErfManager_.GetHostErf(CurDoc_ - 1, nullptr);
        HasHit_ = false;
        return true;
    }

     bool ReadDoc(ui32* docId) {
        if (!Advance())
            return false;
        HasHit_ = true;
        *docId = CurDoc_ - 1;
        return true;
    }

    TProgress Progress() const {
        return TProgress(CurDoc_, Size_);
    }

private:
    bool Advance() {
        CurDoc_++;
        return CurDoc_ != Size_ + 1;
    }

private:
    THostErfManager ErfManager_;
    bool HasHit_ = false;
    ui32 CurDoc_ = 0;
    ui32 Size_ = 0;
};


} //namespace NDoom
