#pragma once

#include "translatedict.h"
#include <library/cpp/streams/base64/base64stream.h>
#include <util/stream/zlib.h>
#include <util/generic/noncopyable.h>

class TTranslationDictFromBin : TNonCopyable {
private:
    TTranslationDict Dict;

public:
    TTranslationDictFromBin(const char* (*getLine)(size_t), size_t defaultLength) {
        TStringDataInputStream instr(getLine, defaultLength);
        TZLibDecompress zstr(&instr);
        Dict.Load(zstr);
    }

    const TTranslationDict& operator()() const {
        return Dict;
    }
};
