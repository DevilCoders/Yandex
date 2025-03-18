#pragma once

#include <util/generic/hash.h>
#include <util/generic/strbuf.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>


namespace NAntiRobot {


class TPageTemplate {
public:
    enum class EEscapeMode {
        Html,
        Json,
    };
    TString ResourceKey;

public:
    TPageTemplate(
        TString body,
        TString resourceKey,
        EEscapeMode escapeMode = EEscapeMode::Html
    );

    TString Gen(const THashMap<TStringBuf, TStringBuf>& params) const;

private:
    struct TChunk {
        bool Verbatim;
        TStringBuf Value;
    };

    TString Body;
    TVector<TChunk> Chunks;
    EEscapeMode EscapeMode;
};


}
