#pragma once

#include "index_format.h"

#include <util/generic/yexception.h>
#include <util/stream/input.h>
#include <util/stream/file.h>

#include <kernel/doom/info/index_info.h>

namespace NDoom {


template<EIndexFormat format, class Base>
class TInfoIndexReader: public Base {
public:
    TInfoIndexReader() = default;

    template<class... Args>
    TInfoIndexReader(IInputStream* infoInput, Args&&... args) {
        Reset(infoInput, std::forward<Args>(args)...);
    }

    template<class... Args>
    TInfoIndexReader(const TString& infoPath, Args&&... args) {
        Reset(infoPath, std::forward<Args>(args)...);
    }

    template<class... Args>
    TInfoIndexReader(const TIndexInfo& indexInfo, Args&&... args)
        : Base(std::forward<Args>(args)...)
        , Info_(indexInfo)
    {
    }

    template<class... Args>
    void Reset(IInputStream* infoInput, Args&&... args) {
        Base::Reset(std::forward<Args>(args)...);
        CheckInfo(infoInput);
    }

    template<class... Args>
    void Reset(const TString& infoPath, Args&&... args) {
        Base::Reset(std::forward<Args>(args)...);
        CheckInfo(infoPath);
    }

    const TIndexInfo& Info() const {
        return Info_;
    }

private:
    void CheckInfo(const TString& infoPath) {
        if (!infoPath)
            return; /* Empty string means "skip check". */

        TIFStream infoInput(infoPath);
        CheckInfo(&infoInput);
    }

    void CheckInfo(IInputStream* infoInput) {
        if (!infoInput)
            return; /* NULL means "skip check". */

        Info_ = LoadIndexInfo(infoInput);

        TString properFormat = ToString(format);
        if (Info_.GetFormat() != properFormat)
            ythrow yexception() << "Invalid index format, expected '" << properFormat << "', got '" << Info_.GetFormat() << "'.";

        CheckHitModel(&Info_, this);
        CheckKeyModel(&Info_, this);
    }

    template<class Writer>
    static void CheckHitModel(TIndexInfo* info, Writer*, typename Writer::THitModel* = nullptr) {
        TString properHitModelType = Writer::THitModel::TypeName();
        if (info->GetHitModelType() != properHitModelType)
            ythrow yexception() << "Invalid hit model type, expected '" << properHitModelType << "', got '" << info->GetHitModelType() << "'.";
    }

    static void CheckHitModel(TIndexInfo*, void*) {}

    template<class Writer>
    static void CheckKeyModel(TIndexInfo* info, Writer*, typename Writer::TKeyModel* = nullptr) {
        TString properKeyModelType = Writer::TKeyModel::TypeName();
        if (info->GetKeyModelType() != properKeyModelType)
            ythrow yexception() << "Invalid key model type, expected '" << properKeyModelType << "', got '" << info->GetKeyModelType() << "'.";
    }

    static void CheckKeyModel(TIndexInfo*, void*) {}

private:
    TIndexInfo Info_;
};


} // namespace NDoom
