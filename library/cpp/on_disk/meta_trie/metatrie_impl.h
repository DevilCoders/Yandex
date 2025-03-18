#pragma once

#include "metatrie.h"

namespace NMetatrie {
    struct TKeyImpl : TAtomicRefCount<TKeyImpl>, TNonCopyable {
        TStringBuf Key;

        TString Str;
        TBuffer Buf;
    };

    struct TValImpl : TAtomicRefCount<TValImpl>, TNonCopyable {
        TStringBuf Val;

        TString Str;
        TBuffer Buf;
        TBuffer BufAux;
    };

}
