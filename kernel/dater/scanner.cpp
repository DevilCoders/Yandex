#include "scanner.h"

namespace ND2 {
namespace NImpl {

TScannerBase::TScannerBase()
    : p()
    , p0()
    , pe()
    , eof()
    , ret()
    , ts()
    , te()
    , cs()
    , act()
{
}

void TScannerBase::SetInput(const wchar16* b, const wchar16* e) {
    p0 = b;
    pe = eof = e;
    ResetPosition();
}

void TScannerBase::ResetPosition() {
    p = p0;
    ret = ts = te = nullptr;
    cs = act = 0;
    Current.Reset();
    Chunks.clear();
    CurrentLanguage = LANG_UNK;
}

void TScannerBase::SkipFirstAndRescan() {
//    Clog << (int)CurrentScannerType << " " << "Skip first and rescan: " << TWtringBuf(ts, te) << Endl;
    for (ui32 i = 0; i < Current.SpanCount(); ++i) {
        if (Current[i].IsMeaningfull()) {
            p = Current[i].End - 1;
            break;
        }
    }

    ResetChunk();
}

void TScannerBase::Accept(EPatternType pt, ui32 ss0, ui32 ss1, ui32 ss2, ui32 ss3, ui32 ss4) {
    if (Chunks.size() > MAX_CHUNKS_IN_SINGLE_SCAN)
        return;

    while (!Current.NoSpans() && Current.BackSpan().Empty()) {
        if (Current.BackSpan().Begin && !Current.BackSpan().End) {
            Current.BackSpan().End = te;
        } else {
            Current.PopSpan();
        }
    }

    // ragel bug workaround
    if (!Current.NoSpans() && Current.BackSpan().Empty() && Current.BackSpan().Begin)
        Current.BackSpan().End = te;

    if (EqualToOneOf(pt, PT_T_DIG_JUNK, PT_U_DIG_JUNK, PT_T_DIG_TIME, PT_T_WRD_TIME)) {
        Current.Reset();
        Current.PushSpan();
        Current[0].Begin = ts;
        Current[0].End = te;
        Current[0].Meaning = ss0;
    } else {
        ui32 i = 0;

        ui32 ss[] = {ss0, ss1, ss2, ss3, ss4, 0};


        for (ui32 j = 0; ss[j]; ++j) {
            while (i < Current.SpanCount() && (!Current[i].IsMeaningfull() || Current[i].HasMeaning<SS_TIME>()))
                ++i;

            if (i >= Current.SpanCount())
                break;

            Current[i].Meaning |= ss[j];
            ++i;
        }
    }

    Current.PatternType = pt;

    if (Current.IsMeaningfull()) {
        Current.Language = CurrentLanguage;
        Chunks.push_back(Current);
    }

    ResetChunk();
    Ret();
}

bool TUrlIdScanner::Scan(const wchar16* b, const wchar16* e, ECountryType ct, ui32 idxyear) {
    SetInput(b, e);

    ResetPosition();
    ScanYYYYMMDDxxx();

    if (!Chunks.empty())
        DisambiguateChunk(Chunks.front(), ct, idxyear, true);

    if (!Chunks.empty() && Chunks.front().HasMeaning<CS_DATE>()) {
        return true;
    }

    ResetPosition();
    ScanxxxYYYYMMDD();

    if (!Chunks.empty())
        DisambiguateChunk(Chunks.front(), ct, idxyear, true);

    if (!Chunks.empty() && Chunks.front().HasMeaning<CS_DATE>()) {
        return true;
    }

    ResetPosition();
    ScanxxxAABBYYYY();

    if (!Chunks.empty())
        DisambiguateChunk(Chunks.front(), ct, idxyear, true);

    if (!Chunks.empty() && Chunks.front().HasMeaning<CS_DATE>()) {
        return true;
    }

    ResetPosition();
    ScanAABBYYYYxxx();

    if (!Chunks.empty())
        DisambiguateChunk(Chunks.front(), ct, idxyear, true);

    if (!Chunks.empty() && Chunks.front().HasMeaning<CS_DATE>()) {
        return true;
    }

    return false;
}

TScanner::TScanner()
{
    SetContext(nullptr, nullptr, TWtringBuf());
}

void TScanner::SetContext(TDaterDocumentContext* ctx, TChunks* res, TWtringBuf b) {
    ResChunks = res;
    Document = ctx;
    SetInput(b.begin(), b.end() + 1);
    ResetPosition();
}

void TScanner::ScanHost() {
    DoScanHost();
    ResChunks->assign(Chunks.begin(), Chunks.end());
    RemoveDuplicates();
    DisambiguateChunks(false);
}

void TScanner::ScanPath() {
    DoScanSegment(LT_URL_PATH, true);
}

void TScanner::ScanQuery() {
    DoScanSegment(LT_URL_QUERY, true);
}

void TScanner::ScanText(bool withranges) {
    DoScanSegment(LT_TEXT, withranges);
}

void TScanner::DoScanSegment(ELocationType lt, bool withranges) {
    bool url = EqualToOneOf(lt, LT_URL_PATH, LT_URL_QUERY);

    ResChunks->clear();

    if (url) {
        DoScanInLang(Document->MainLanguage, ST_URL_DIGITS);
    } else {
        DoScanInLang(Document->MainLanguage, ST_TEXT_DIGIT_DATES);
    }

    if (url) {
        DoScanInLang(LANG_ENG, ST_URL_WORDS);
    } else {
        DoScanInLang(LANG_ENG, ST_TEXT_WORD_DATES);

        if (withranges) {
            DoScanInLang(LANG_ENG, ST_TEXT_WORD_RANGES);
        }
    }

    if (!EqualToOneOf(Document->MainLanguage, LANG_UNK, LANG_ENG)) {
        if (url) {
            DoScanInLang(Document->MainLanguage, ST_URL_WORDS);
        } else {
            DoScanInLang(Document->MainLanguage, ST_TEXT_WORD_DATES);

            if (withranges) {
                DoScanInLang(Document->MainLanguage, ST_TEXT_WORD_RANGES);
            }
        }
    }

    if (!EqualToOneOf(Document->AuxLanguage, LANG_UNK, LANG_ENG, Document->MainLanguage)) {
        if (url) {
            DoScanInLang(Document->AuxLanguage, ST_URL_WORDS);
        } else {
            DoScanInLang(Document->AuxLanguage, ST_TEXT_WORD_DATES);

            if (withranges) {
                DoScanInLang(Document->AuxLanguage, ST_TEXT_WORD_RANGES);
            }
        }
    }

    RemoveDuplicates();
    DisambiguateChunks(LT_URL_PATH == lt);
}

void TScanner::DoScanInLang(ELanguage lang, EScannerType stype) {
    ResetPosition();

    if (stype != ST_TEXT_DIGIT_DATES && stype != ST_URL_DIGITS) {
        switch (lang) {
        default:
            break;
        case LANG_BEL:
            switch (stype) {
            default:
                break;
            case ST_URL_WORDS:
                DoScanUrlWords<LANG_BEL>();
                break;
            case ST_TEXT_WORD_DATES:
                DoScanTextWordDates<LANG_BEL>();
                break;
            case ST_TEXT_WORD_RANGES:
                DoScanTextWordRanges<LANG_BEL>();
                break;
            };
            break;
        case LANG_CAT:
            switch (stype) {
            default:
                break;
            case ST_URL_WORDS:
                DoScanUrlWords<LANG_CAT>();
                break;
            case ST_TEXT_WORD_DATES:
                DoScanTextWordDates<LANG_CAT>();
                break;
            case ST_TEXT_WORD_RANGES:
                DoScanTextWordRanges<LANG_CAT>();
                break;
            };
            break;
        case LANG_CZE:
            switch (stype) {
            default:
                break;
            case ST_URL_WORDS:
                DoScanUrlWords<LANG_CZE>();
                break;
            case ST_TEXT_WORD_DATES:
                DoScanTextWordDates<LANG_CZE>();
                break;
            case ST_TEXT_WORD_RANGES:
                DoScanTextWordRanges<LANG_CZE>();
                break;
            };
            break;
        case LANG_GER:
            switch (stype) {
            default:
                break;
            case ST_URL_WORDS:
                DoScanUrlWords<LANG_GER>();
                break;
            case ST_TEXT_WORD_DATES:
                DoScanTextWordDates<LANG_GER>();
                break;
            case ST_TEXT_WORD_RANGES:
                DoScanTextWordRanges<LANG_GER>();
                break;
            };
            break;
        case LANG_ENG:
            switch (stype) {
            default:
                break;
            case ST_URL_WORDS:
                DoScanUrlWords<LANG_ENG>();
                break;
            case ST_TEXT_WORD_DATES:
                DoScanTextWordDates<LANG_ENG>();
                break;
            case ST_TEXT_WORD_RANGES:
                DoScanTextWordRanges<LANG_ENG>();
                break;
            };
            break;
        case LANG_SPA:
            switch (stype) {
            default:
                break;
            case ST_URL_WORDS:
                DoScanUrlWords<LANG_SPA>();
                break;
            case ST_TEXT_WORD_DATES:
                DoScanTextWordDates<LANG_SPA>();
                break;
            case ST_TEXT_WORD_RANGES:
                DoScanTextWordRanges<LANG_SPA>();
                break;
            };
            break;
        case LANG_FRE:
            switch (stype) {
            default:
                break;
            case ST_URL_WORDS:
                DoScanUrlWords<LANG_FRE>();
                break;
            case ST_TEXT_WORD_DATES:
                DoScanTextWordDates<LANG_FRE>();
                break;
            case ST_TEXT_WORD_RANGES:
                DoScanTextWordRanges<LANG_FRE>();
                break;
            };
            break;
        case LANG_ITA:
            switch (stype) {
            default:
                break;
            case ST_URL_WORDS:
                DoScanUrlWords<LANG_ITA>();
                break;
            case ST_TEXT_WORD_DATES:
                DoScanTextWordDates<LANG_ITA>();
                break;
            case ST_TEXT_WORD_RANGES:
                DoScanTextWordRanges<LANG_ITA>();
                break;
            };
            break;
        case LANG_KAZ:
            switch (stype) {
            default:
                break;
            case ST_URL_WORDS:
                DoScanUrlWords<LANG_KAZ>();
                break;
            case ST_TEXT_WORD_DATES:
                DoScanTextWordDates<LANG_KAZ>();
                break;
            case ST_TEXT_WORD_RANGES:
                DoScanTextWordRanges<LANG_KAZ>();
                break;
            };
            break;
        case LANG_POL:
            switch (stype) {
            default:
                break;
            case ST_URL_WORDS:
                DoScanUrlWords<LANG_POL>();
                break;
            case ST_TEXT_WORD_DATES:
                DoScanTextWordDates<LANG_POL>();
                break;
            case ST_TEXT_WORD_RANGES:
                DoScanTextWordRanges<LANG_POL>();
                break;
            };
            break;
        case LANG_RUS:
            switch (stype) {
            default:
                break;
            case ST_URL_WORDS:
                DoScanUrlWords<LANG_RUS>();
                break;
            case ST_TEXT_WORD_DATES:
                DoScanTextWordDates<LANG_RUS>();
                break;
            case ST_TEXT_WORD_RANGES:
                DoScanTextWordRanges<LANG_RUS>();
                break;
            };
            break;
        case LANG_TUR:
            switch (stype) {
            default:
                break;
            case ST_URL_WORDS:
                DoScanUrlWords<LANG_TUR>();
                break;
            case ST_TEXT_WORD_DATES:
                DoScanTextWordDates<LANG_TUR>();
                break;
            case ST_TEXT_WORD_RANGES:
                DoScanTextWordRanges<LANG_TUR>();
                break;
            };
            break;
        case LANG_UKR:
            switch (stype) {
            default:
                break;
            case ST_URL_WORDS:
                DoScanUrlWords<LANG_UKR>();
                break;
            case ST_TEXT_WORD_DATES:
                DoScanTextWordDates<LANG_UKR>();
                break;
            case ST_TEXT_WORD_RANGES:
                DoScanTextWordRanges<LANG_UKR>();
                break;
            };
            break;
        }
    } else {
        switch (stype) {
        default:
            break;
        case ST_URL_DIGITS:
            DoScanUrlDigits();
            break;
        case ST_TEXT_DIGIT_DATES:
            DoScanTextDigitDates();
            break;
        }
    }

    ResChunks->insert(ResChunks->end(), Chunks.begin(), Chunks.end());
}

static bool Intersect(const TChunk& a, const TChunk& b) {
    // empty or do not intersect at all
    if (!a.Size() || !b.Size() || b.End() <= a.Begin() || a.End() <= b.Begin())
        return false;

    const wchar16* abeg = a.MeaningfullBegin();
    const wchar16* aend = a.MeaningfullEnd();

    const wchar16* bbeg = b.MeaningfullBegin();
    const wchar16* bend = b.MeaningfullEnd();

    return bbeg >= abeg && bbeg < aend || abeg >= bbeg && abeg < bend;
}

template <bool End>
struct TEndLess {
    bool operator()(const TChunk& a, const TChunk& b) const { return End ? a.End() < b.End() : a.Begin() < b.Begin(); }
    bool operator()(const wchar16* a,  const TChunk& b) const { return End ?       a < b.End() :         a < b.Begin(); }
    bool operator()(const TChunk& a, const wchar16* b ) const { return End ? a.End() < b       : a.Begin() < b; }
};

void TScanner::RemoveDuplicates() {
    i64 maxsz = 0;

    for (TChunks::const_iterator it = ResChunks->begin(); it != ResChunks->end(); ++it) {
        maxsz = Max<i64>(it->Size(), maxsz);
    }

    Sort(ResChunks->begin(), ResChunks->end(), TEndLess<true>());

    TmpChunks.clear();
    TmpChunks.reserve(Chunks.size());
    for (TChunks::const_iterator it = ResChunks->begin(); it != ResChunks->end(); ++it) {
        if (!it->IsMeaningfull())
            continue;

        bool add = true;
        TChunks::iterator rit = UpperBound(TmpChunks.begin(), TmpChunks.end(), it->Begin(), TEndLess<true>());
        for (; rit != TmpChunks.end() && rit->End() - it->Begin() <= 3 * maxsz; ++rit) {
            if (Intersect(*rit, *it)) {
//                Clog << *rit << "(" << (int)rit->PatternType << ") x (" << (int)it->PatternType << ")" << *it << Endl;
                if (rit->Size() < it->Size()) {
                    rit->Reset(CS_JUNK, SS_NONE);
                } else {
                    add = false;
                }
            }
        }

        if (add)
            TmpChunks.push_back(*it);
    }

    Sort(TmpChunks.begin(), TmpChunks.end(), TEndLess<false>());

    ResChunks->clear();

    for (TChunks::const_iterator it = TmpChunks.begin(); it != TmpChunks.end(); ++it) {
        if (it->IsMeaningfull())
            ResChunks->push_back(*it);
    }
}

void TScanner::DisambiguateChunks(bool urlpath) {
    for (TChunks::iterator it = ResChunks->begin(); it != ResChunks->end(); ++it) {
        if (PT_U_ID == it->PatternType) {
            if (UrlIdScanner.Scan(it->Begin(), it->End(), Document->CountryType, Document->IndexYear))
                *it = UrlIdScanner.GetChunks().front();
            else
                SetJunk(*it);
        } else {
            DisambiguateChunk(*it, Document->CountryType, Document->IndexYear, urlpath);
        }
    }
}

}
}
