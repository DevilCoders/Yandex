#pragma once

#include <kernel/tarc/docdescr/docdescr.h>

#include <util/generic/hash.h>
#include <util/generic/strbuf.h>
#include <util/generic/string.h>

namespace NSnippets {
    struct TStaticData {
        typedef THashMap<TString, TUtf16String> TAttrs;
        TAttrs Attrs;
        bool Absent = true;
        TStaticData(const TDocInfos& infos, const TString& infoName);
        explicit TStaticData(const TStringBuf& infoValue);
        bool TryGetAttr(const TString& key, TUtf16String& dest) const;
    };
}

