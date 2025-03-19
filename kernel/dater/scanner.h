#pragma once

#include "chunk.h"
#include "document.h"

#include <util/draft/datetime.h>

namespace ND2 {
namespace NImpl {

enum ELocationType {
    LT_URL_PATH,
    LT_URL_QUERY,
    LT_TEXT,
};

enum EScannerType {
    ST_UNKNOWN = 0,
    ST_URL_DIGITS,
    ST_URL_WORDS,
    ST_TEXT_DIGIT_DATES,
    ST_TEXT_WORD_DATES,
    ST_TEXT_WORD_RANGES,
};

class TScannerBase {
protected:
    ELanguage CurrentLanguage = LANG_UNK;

    const wchar16* p;
    const wchar16* p0;
    const wchar16* pe;
    const wchar16* eof;
    const wchar16* ret;
    const wchar16* ts;
    const wchar16* te;

    int cs;
    int act;

    TChunk Current;
    TChunks Chunks;

protected:

    TScannerBase();

    void SetInput(const wchar16* b, const wchar16* e);

    void Accept(EPatternType, ui32, ui32 = SS_NONE, ui32 = SS_NONE, ui32 = SS_NONE, ui32 = SS_NONE);

    void Reject() {
        ResetChunk();
        Ret();
    }

    bool AtStart() const {
        return ts == p0;
    }

    bool AtEnd() const {
        return p + 1 == pe;
    }

    void ResetChunk() {
        Current.Reset();
    }

    void Hold() {
        ret = p;
    }

    void Ret() {
        CurrSpan().End = p;

        if (ret) {
            te = p = ret - 1;
            ret = nullptr;
        }
    }

    void In() {
        CurrSpan().End = p;
        NextSpan().Begin = p;
    }

    void Out() {
        CurrSpan().End = p;
    }

    TChunkSpan& NextSpan() {
        return Current.PushSpan();
    }

    TChunkSpan& CurrSpan() {
        return Current.BackSpan();
    }

    TChunkSpan& PrevSpan() {
        return Current.PrevSpan();
    }

    bool HasSpan() const {
        return !Current.NoSpans();
    }

    bool HasPrevSpan() const {
        return Current.SpanCount() > 1;
    }

    void SetMon(ui32 v) {
        TChunkSpan& sp = CurrSpan();
        sp.Meaning = SS_MONTH | SS_WRD;
        sp.Value = v;
    }

    template <ESpanSemantics sem>
    void AddMeaning() {
        CurrSpan().AddMeaning<sem>();
    }

    void SkipFirstAndRescan();

    void ResetPosition();
};

class TUrlIdScanner : public TScannerBase {
public:
    bool Scan(const wchar16* b, const wchar16* e, ECountryType ct, ui32 idxyear);

    TChunks& GetChunks() {
        return Chunks;
    }

    void ScanYYYYMMDDxxx();
    void ScanxxxYYYYMMDD();
    void ScanxxxAABBYYYY();
    void ScanAABBYYYYxxx();
};

// ahtung!!! looks one simbol past the end of the buffer. expects to find '\0' there.
class TScanner : public TScannerBase {
protected:
    TUrlIdScanner UrlIdScanner;

    TDaterDocumentContext* Document;
    TChunks* ResChunks;

    TChunks TmpChunks;

public:

    TScanner();

    void SetContext(TDaterDocumentContext* ctx, TChunks* res, TWtringBuf str);

    void PreScanUrl();

    void ScanHost();
    void ScanPath();
    void ScanQuery();
    void ScanText(bool withranges);

    void Process();

    // todo: simple api for url dates in snippets (need to denormalize to original string in the end)
protected:
    void SetDownloadUrl() {
        Document->Flags.UrlHasDownloadPattern = true;
    }

    template <ELanguage lang>
    bool IsLang() {
        return EqualToOneOf(lang, Document->MainLanguage, Document->AuxLanguage);
    }

    void RemoveDuplicates();
    void DisambiguateChunks(bool urlpath);

    void DoScanHost();
    void DoScanSegment(ELocationType, bool withranges);

    void DoScanUrlDigits();
    void DoScanTextDigitDates();

    void DoScanInLang(ELanguage lang, EScannerType stype);

    template <ELanguage lang>
    void DoScanUrlWords();

    template <ELanguage lang>
    void DoScanTextWordDates();

    template <ELanguage lang>
    void DoScanTextWordRanges();

};

template <typename Char>
inline ui64 Atoi(const Char* b, const Char* e) {
    ui64 res = 0;

    for (; b != e && *b >= '0' && *b <= '9'; ++b)
        res = res * 10 + (*b - '0');

    return b == e ? res : ui64(-1);
}

inline ui64 Atoi(const TChunkSpan& s) {
    return Atoi(s.Begin, s.End);
}

inline ui64 Atoi(const TStringBuf& s) {
    return Atoi(s.begin(), s.end());
}

inline ui64 Atoi(const TWtringBuf& s) {
    return Atoi(s.begin(), s.end());
}

bool DisambiguateChunk(TChunk& ch, ECountryType ct, ui32 idxyear, bool frompath = false);
bool SetJunk(TChunk& ch);

}
}
