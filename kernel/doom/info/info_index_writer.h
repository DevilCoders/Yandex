#pragma once

#include <util/stream/output.h>
#include <util/stream/file.h>
#include <util/string/cast.h>

#include <kernel/doom/info/index_format.h>
#include <kernel/doom/info/index_info.h>

namespace NDoom {


template<EIndexFormat format, class Base>
class TInfoIndexWriter: public Base {
public:
    TInfoIndexWriter() = default;

    template<class... Args>
    TInfoIndexWriter(IOutputStream* infoOutput, Args&&... args) {
        Reset(infoOutput, std::forward<Args>(args)...);
    }

    template<class... Args>
    TInfoIndexWriter(const TString& infoPath, Args&&... args) {
        Reset(infoPath, std::forward<Args>(args)...);
    }

    ~TInfoIndexWriter() {
        Finish();
    }

    template<class... Args>
    void Reset(IOutputStream* infoOutput, Args&&... args) {
        Base::Reset(std::forward<Args>(args)...);
        LocalInfoOutput_.Reset();
        InfoOutput_ = infoOutput;
        ResetInfo();
    }

    template<class... Args>
    void Reset(const TString& infoPath, Args&&... args) {
        Base::Reset(std::forward<Args>(args)...);
        LocalInfoOutput_.Reset(new TOFStream(infoPath));
        InfoOutput_ = LocalInfoOutput_.Get();
        ResetInfo();
    }

    void Finish() {
        if (Base::IsFinished())
            return;

        Base::Finish();
        SaveIndexInfo(InfoOutput_, Info_);
        if (LocalInfoOutput_)
            LocalInfoOutput_->Finish();
    }

    TIndexInfo& Info() {
        return Info_;
    }

private:
    void ResetInfo() {
        Info_ = MakeDefaultIndexInfo();
        Info_.SetFormat(ToString(format));

        /* Use ADL to initialize model types if base class has them. */
        ResetInfoHitModel(&Info_, this);
        ResetInfoKeyModel(&Info_, this);
    }

    template<class Writer>
    static void ResetInfoHitModel(TIndexInfo* info, Writer*, typename Writer::THitModel* = nullptr) {
        info->SetHitModelType(Writer::THitModel::TypeName());
    }

    static void ResetInfoHitModel(TIndexInfo*, void*) {}

    template<class Writer>
    static void ResetInfoKeyModel(TIndexInfo* info, Writer*, typename Writer::TKeyModel* = nullptr) {
        info->SetKeyModelType(Writer::TKeyModel::TypeName());
    }

    static void ResetInfoKeyModel(TIndexInfo*, void*) {}

private:
    THolder<IOutputStream> LocalInfoOutput_;
    TIndexInfo Info_;
    IOutputStream* InfoOutput_ = nullptr;
};


} // namespace NDoom
