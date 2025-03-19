#include <string.h>

#include "dater.h"
#include <util/charset/unidata.h>

namespace ND2 {

void TDaterScanContext::Reset() {
    ResetCurrent();
    UrlDates.clear();
    TitleDates.clear();
    TextDates.clear();
}

void TDaterScanContext::ResetCurrent() {
    CurrentText.clear();
    CurrentCoords.clear();
    CurrentChunks.clear();
}

struct TCoordCmpEnd {
    bool operator()(const NSegm::TCoord& a, const NSegm::TCoord& b) const {
        return a.End < b.End;//(a.Begin + (a.End - a.Begin) / 2) < (b.Begin + (b.End - b.Begin) / 2 );
    }
};

struct TCoordCmpBegin {
    bool operator()(const NSegm::TCoord& a, const NSegm::TCoord& b) const {
        return a.Begin > b.Begin;//(a.Begin + (a.End - a.Begin) / 2) < (b.Begin + (b.End - b.Begin) / 2 );
    }
};

static NSegm::TSpan FindPositions(const NImpl::TChunk& ch, const NSegm::TPosCoords& pos, TWtringBuf wholetext) {
    if (pos.empty())
        return NSegm::TSpan();

    ptrdiff_t beg = ch.MeaningfullBegin() - wholetext.begin();
    ptrdiff_t end = ch.MeaningfullEnd() - wholetext.begin();

    NSegm::TPosCoord b, e;

    size_t off = 0;

    {
        NSegm::TCoord c(beg, beg);
        NSegm::TPosCoords::const_iterator it = UpperBound(pos.begin(), pos.end(), c, TCoordCmpEnd());

        if (it == pos.end()) {
            b = pos.back();
            off = pos.size() - 1;
        } else {
            b = *it;
            off = it - pos.begin();
        }
    }

    {
        bool found = false;

        NSegm::TCoord c(end, end);
        NSegm::TPosCoords::const_reverse_iterator it = LowerBound(pos.rbegin(), pos.rend() - off, c, TCoordCmpBegin());
        if (it != pos.rend()) {
            if (it->Begin != end) {
                if (it != pos.rbegin()) {
                    --it;
                    e = *it;
                    found = true;
                }
            } else {
                e = *it;
                found = true;
            }
        }

        if (!found) {
            e = pos.back();
            e.Pos = e.Pos.NextWord();
        }
    }

    return NSegm::TSpan(b.Pos, e.Pos);
}

// todo: test it!
static void MakeDates(TDates& dates, NImpl::TChunks& chs, const NSegm::TPosCoords& pos,
                      TWtringBuf wholetext, ui32 idxyear, bool acceptNoYear = false)
{
    using namespace NImpl;
    using namespace NSegm;

    if (chs.empty())
        return;

    for (TChunks::iterator it = chs.begin(); it != chs.end(); ++it) {
        if (it->HasMeaning<CS_TIME>() && !it->HasMeaning<CS_DATE>()) {
            const TChunk& ch = *it;
            TChunk* left = (it != chs.begin() ? (it - 1) : nullptr);
            TChunk* right = (it + 1 != chs.end() ? (it + 1) : nullptr);

            if (left && (left->HasMeaning<CS_TIME>() || !left->HasMeaning<CS_DATE>()))
                left = nullptr;

            if (right && (right->HasMeaning<CS_TIME>() || !right->HasMeaning<CS_DATE>()))
                right = nullptr;

            ui32 leftdist = (left ? ch.MeaningfullBegin() - left->MeaningfullEnd() : -1);
            ui32 rightdist = (right ? right->MeaningfullBegin() - ch.MeaningfullEnd() : -1);

            if (Min(leftdist, rightdist) > MAX_DATE_TIME_DIST)
                continue;

            if (left && leftdist <= rightdist)
                left->AddMeaning<CS_TIME>();
            else if (right)
                right->AddMeaning<CS_TIME>();
        }
    }

    for (TChunks::const_iterator it = chs.begin(); it != chs.end(); ++it) {
        const TChunk& ch = *it;

        NImpl::TValidDateProfile pr(ch.PatternType);

        if (ch.HasMeaning<CS_DATE>()) {
            TDate d = FindPositions(ch, pos, wholetext);
            d.Language = ch.Language;
            d.Features.HasTime = ch.HasMeaning<CS_TIME>();
            d.Features.FromUrlId = pr.IsId;
            d.Features.FromHost = NImpl::PT_H_YYYY == ch.PatternType;
            d.Features.FromAmbiguousPattern = pr.IsAmbiguous;
            d.Features.MonthIsWord = pr.MonthIsWord;

            const ui32 sz = ch.SpanCount();
            for (ui32 i = 0; i < sz; ++i) {
                if (ch[i].HasMeaning<SS_DAY>()) {
                    d.Data.Day = ch[i].Value;
                    d.Features.HasDay = true;
                } else if (ch[i].HasMeaning<SS_MONTH>()) {
                    d.Data.Month = ch[i].Value;
                    d.Features.HasMonth = true;
                } else if (ch[i].HasMeaning<SS_YEAR>()) {
                    d.Data.Year = ch[i].Value;
                    d.Features.HasYear = true;
                    d.Features.Has4DigitsInYear = 4 == ch[i].Size();

                    // TODO: better logic here
                    if (!d.Data.Year && d.Features.HasTime)
                        d.Data.Year = idxyear;
                }
            }

            // todo: better filtering
            if (d.Data.Year || acceptNoYear)
                dates.push_back(d);
        } else if (ch.HasMeaning<CS_DATE_RANGE>()) {
            TDate a = FindPositions(ch, pos, wholetext);
            TDate b = FindPositions(ch, pos, wholetext);
            a.Features.FromDateRange = true;
            b.Features.FromDateRange = true;
            a.Features.MonthIsWord = b.Features.MonthIsWord = pr.MonthIsWord;

            const ui32 sz = ch.SpanCount();
            switch (ch.PatternType) {
            default:
                continue;
            case PT_U_RNG_MMMMYY:
            case PT_U_RNG_YYMMMM:
                a.Features.HasMonth = b.Features.HasMonth = true;
                a.Features.HasYear  = b.Features.HasYear = true;

                for (ui32 i = 0; i < sz; ++i) {
                    if (ch[i].HasMeaning<SS_MONTH>()) {
                        if (a.Data.Month)
                            b.Data.Month = ch[i].Value;
                        else
                            a.Data.Month = ch[i].Value;
                    } else if (ch[i].HasMeaning<SS_YEAR>()) {
                        a.Data.Year = b.Data.Year = ch[i].Value;
                        a.Features.Has4DigitsInYear = b.Features.Has4DigitsInYear = 4 == ch[i].Size();
                    }
                }

                break;
            case PT_T_RNG_DDMM_DDMMYYYY:
            case PT_T_RNG_MMDD_MMDDYYYY:
                a.Features.HasYear  = b.Features.HasYear = true;
                [[fallthrough]]; // AUTOGENERATED_FALLTHROUGH_FIXME

            case PT_T_RNG_DDMM_DDMM:
            case PT_T_RNG_MMDD_MMDD:
                a.Features.HasDay   = b.Features.HasDay = true;
                a.Features.HasMonth = b.Features.HasMonth = true;

                for (ui32 i = 0; i < sz; ++i) {
                    if (ch[i].HasMeaning<SS_DAY>()) {
                        if (a.Data.Day)
                            b.Data.Day = ch[i].Value;
                        else
                            a.Data.Day = ch[i].Value;
                    } else if (ch[i].HasMeaning<SS_MONTH>()) {
                        if (a.Data.Month)
                            b.Data.Month = ch[i].Value;
                        else
                            a.Data.Month = ch[i].Value;
                    } else if (ch[i].HasMeaning<SS_YEAR>()) {
                        a.Data.Year = b.Data.Year = ch[i].Value;
                        a.Features.Has4DigitsInYear = b.Features.Has4DigitsInYear = 4 == ch[i].Size();
                    }
                }
                break;
            case PT_T_RNG_MM_MMYYYY:
                a.Features.HasYear  = b.Features.HasYear = true;
                [[fallthrough]]; // AUTOGENERATED_FALLTHROUGH_FIXME

            case PT_T_RNG_MM_MM:
                a.Features.HasMonth = b.Features.HasMonth = true;

                for (ui32 i = 0; i < sz; ++i) {
                    if (ch[i].HasMeaning<SS_MONTH>()) {
                        if (a.Data.Month)
                            b.Data.Month = ch[i].Value;
                        else
                            a.Data.Month = ch[i].Value;
                    } else if (ch[i].HasMeaning<SS_YEAR>()) {
                        a.Data.Year = b.Data.Year = ch[i].Value;
                        a.Features.Has4DigitsInYear = b.Features.Has4DigitsInYear = 4 == ch[i].Size();
                    }
                }

                break;
            case PT_T_RNG_MMYYYY_MMYYYY:
            case PT_T_RNG_YYYYMM_YYYYMM:
                a.Features.HasYear = b.Features.HasYear = true;
                a.Features.HasMonth = b.Features.HasMonth = true;

                for (ui32 i = 0; i < sz; ++i) {
                    if (ch[i].HasMeaning<SS_MONTH>()) {
                        if (a.Data.Month)
                            b.Data.Month = ch[i].Value;
                        else
                            a.Data.Month = ch[i].Value;
                    } else if (ch[i].HasMeaning<SS_YEAR>()) {
                        if (a.Data.Year) {
                            b.Data.Year = ch[i].Value;
                            b.Features.Has4DigitsInYear = 4 == ch[i].Size();
                        } else {
                            a.Data.Year = ch[i].Value;
                            a.Features.Has4DigitsInYear = 4 == ch[i].Size();
                        }
                    }
                }

                break;
            case PT_T_RNG_YYYYMM_YYYY:
            case PT_T_RNG_MMYYYY_YYYY:
                a.Features.HasYear  = b.Features.HasYear = true;
                a.Features.HasMonth = true;

                for (ui32 i = 0; i < sz; ++i) {
                    if (ch[i].HasMeaning<SS_MONTH>()) {
                        a.Data.Month = ch[i].Value;
                    } else if (ch[i].HasMeaning<SS_YEAR>()) {
                        if (a.Data.Year) {
                            b.Data.Year = ch[i].Value;
                            b.Features.Has4DigitsInYear = 4 == ch[i].Size();
                        } else {
                            a.Data.Year = ch[i].Value;
                            a.Features.Has4DigitsInYear = 4 == ch[i].Size();
                        }
                    }
                }

                break;
            case PT_T_RNG_YYYY_MMYYYY:
            case PT_T_RNG_YYYY_YYYYMM:
                a.Features.HasYear  = b.Features.HasYear = true;
                b.Features.HasMonth = true;

                for (ui32 i = 0; i < sz; ++i) {
                    if (ch[i].HasMeaning<SS_MONTH>()) {
                        b.Data.Month = ch[i].Value;
                    } else if (ch[i].HasMeaning<SS_YEAR>()) {
                        if (a.Data.Year) {
                            b.Data.Year = ch[i].Value;
                            b.Features.Has4DigitsInYear = 4 == ch[i].Size();
                        } else {
                            a.Data.Year = ch[i].Value;
                            a.Features.Has4DigitsInYear = 4 == ch[i].Size();
                        }
                    }
                }

                break;
            case PT_T_RNG_DD_DDMMYYYY:
                a.Features.HasYear  = b.Features.HasYear = true;
                [[fallthrough]]; // AUTOGENERATED_FALLTHROUGH_FIXME

            case PT_T_RNG_DD_DDMM:
                a.Features.HasDay   = b.Features.HasDay = true;
                a.Features.HasMonth = b.Features.HasMonth = true;

                for (ui32 i = 0; i < sz; ++i) {
                    if (ch[i].HasMeaning<SS_DAY>()) {
                        if (a.Data.Day)
                            b.Data.Day = ch[i].Value;
                        else
                            a.Data.Day = ch[i].Value;
                    } else if (ch[i].HasMeaning<SS_MONTH>()) {
                        a.Data.Month = b.Data.Month = ch[i].Value;
                    } else if (ch[i].HasMeaning<SS_YEAR>()) {
                        a.Data.Year = b.Data.Year = ch[i].Value;
                        a.Features.Has4DigitsInYear = b.Features.Has4DigitsInYear = 4 == ch[i].Size();
                    }
                }

                break;
            }

            // todo: better filtering
            if (a.Data.Year && b.Data.Year) {
                dates.push_back(a);
                dates.push_back(b);
            }
        }
    }
}

inline bool UpdateLimits(ui32& scancnt, ui32& chunkcnt, const NImpl::TChunks& ch) {
    --scancnt;
    chunkcnt -= Min<ui32>(ch.size(), chunkcnt);
    return scancnt && chunkcnt;
}

void TDaterScanContext::PrepareForScan() {
    Concatenator.Normalizer.SetLanguages(Document.MainLanguage, Document.AuxLanguage);
}

void TDaterScanContext::PrepareUrl(TWtringBuf obj) {
    ResetCurrent();
    Concatenator.DoNormalize(obj, CurrentText);
    Scanner.SetContext(&Document, &CurrentChunks, CurrentText);
}

void TDaterScanContext::ScanUrl() {
    // host
    PrepareUrl(Document.Url.Host);
    Scanner.ScanHost();
    MakeDates(UrlDates, CurrentChunks, CurrentCoords, CurrentText, Document.IndexYear);

    // path
    PrepareUrl(Document.Url.Path);
    Scanner.PreScanUrl();
    Scanner.ScanPath();
    MakeDates(UrlDates, CurrentChunks, CurrentCoords, CurrentText, Document.IndexYear);

    // query
    PrepareUrl(Document.Url.Query);
    Scanner.PreScanUrl();
    Scanner.ScanQuery();
    MakeDates(UrlDates, CurrentChunks, CurrentCoords, CurrentText, Document.IndexYear);
}

inline bool IsDigitOrDot(wchar16 c) {
    return IsCommonDigit(c) || (c == '.');
}

const wchar16* FindFirstDigitOrDot(const wchar16* start, ui32 len) {
    for (ui32 i = 0; i < len; i++) {
        if (IsDigitOrDot(start[i]))
            return &start[i];
    }
    return nullptr;
}

const wchar16* FindFirstNotDigitOrDot(const wchar16* start, ui32 len) {
    for (ui32 i = 0; i < len; i++) {
        if (!IsDigitOrDot(start[i]))
            return &start[i];
    }
    return &start[len];
}

class TCanBeDateFinder {
public:
    const ui32 DATE_SURROUND = 40;

public:
    const wchar16* Start;
    const wchar16* NextScan;
    ui32 Len;

public:
    TCanBeDateFinder(const wchar16* start, ui32 len)
        : Start(start)
        , NextScan(start)
        , Len(len)
    {
    }

    bool NextDate(const wchar16*& beg, ui32& segLen) {
        do {
            const wchar16* digit = FindFirstDigitOrDot(NextScan, Start + Len - NextScan);
            if (digit == nullptr)
                return false;

            const wchar16* notDigit = FindFirstNotDigitOrDot(digit, Start + Len - digit);
            if (notDigit - digit >= 2) {
                beg = digit - DATE_SURROUND;
                if (beg < Start) {
                    beg = Start;
                }
                const wchar16* finish = notDigit + DATE_SURROUND;
                if (finish > Start + Len) {
                    finish = Start + Len;
                }
                segLen = finish - beg;
                NextScan = notDigit;
                return true;
            }
            NextScan = notDigit;
        } while (true);
    }
};

bool TDaterScanContext::ScanTextRange(NSegm::TRange r, TDates& out, bool withranges) {
    if (r.empty()) {
        return false;
    }

    ResetCurrent();

    Concatenator.SetContext(&Document, &CurrentText, &CurrentCoords);
    r.ConcatenateText(Concatenator);

    if (CurrentText.size() < 4) {
        return false;
    }

    bool huge = CurrentText.size() >= 0x10000;
    TArrayHolder<ui32> coords;
    ui32* coordMap;
    const size_t bufferSize = CurrentText.size() + 1;
    if (huge) {
        coords = TArrayHolder<ui32>(new ui32[bufferSize]);
        coordMap = coords.Get();
    } else {
        coordMap = static_cast<ui32*>(alloca(bufferSize * sizeof(ui32)));
    }

    const wchar16* filteredText = CurrentText.data();
    ui32 filteredLen = CurrentText.size();
    TArrayHolder<wchar16> filtered;
    if (LookForDigits) {
        bool twoSequentialDigits = false;
        for (size_t i = 1; i < CurrentText.size(); ++i) {
            if (IsCommonDigit(CurrentText[i - 1]) && IsCommonDigit(CurrentText[i])) {
                twoSequentialDigits = true;
                break;
            }
        }
        if (!twoSequentialDigits)
            return false;
        TCanBeDateFinder finder(CurrentText.data(), CurrentText.size());
        wchar16* filteredTxt;
        if (huge) {
            filtered = TArrayHolder<wchar16>(new wchar16[CurrentText.size() + 1]);
            filteredTxt = filtered.Get();
        } else {
            filteredTxt = static_cast<wchar16*>(alloca((CurrentText.size() + 1) * sizeof(wchar16) << 1));
        }
        wchar16* pfiltered = filteredTxt;
        const wchar16* prevEnd = nullptr;
        const wchar16* seg;
        ui32 segLen;
        while (finder.NextDate(seg, segLen)) {
            if (seg < prevEnd) {
                segLen -= prevEnd - seg;
                seg = prevEnd;
            }
            if ((pfiltered > filteredTxt) && (seg > prevEnd)) {
                coordMap[pfiltered - filteredTxt] = seg - CurrentText.data();
                *pfiltered++ = '#';
            }
            prevEnd = seg + segLen;
            memcpy(pfiltered, seg, segLen * sizeof(wchar16));
            ui32 start = seg - CurrentText.data();
            ui32 istart = pfiltered - filteredTxt;
            for (ui32 i = 0; i < segLen; i++) {
                coordMap[istart + i] = start + i;
            }
            pfiltered += segLen;
        }
        *pfiltered = 0;
        coordMap[pfiltered - filteredTxt] = CurrentText.size();
        filteredLen = pfiltered - filteredTxt;
        filteredText = filteredTxt;
        if (filteredLen < 4) {
            return twoSequentialDigits;
        }
    }

    Scanner.SetContext(&Document, &CurrentChunks, TWtringBuf(filteredText, filteredLen));
    Scanner.ScanText(withranges);

    if (CurrentChunks.empty()) {
        return true;
    }

    if (LookForDigits) {
        for (auto& chunk: CurrentChunks) {
            for (ui32 i = 0; i < chunk.SpanCount(); i++) {
                chunk[i].Begin = CurrentText.data() + coordMap[chunk[i].Begin - filteredText];
                if (chunk[i].End - filteredText > filteredLen) {
                    chunk[i].End = CurrentText.data() + coordMap[filteredLen] + (chunk[i].End - filteredText - filteredLen);
                } else {
                    chunk[i].End = CurrentText.data() + coordMap[chunk[i].End - filteredText];
                }
            }
        }
    }
    MakeDates(out, CurrentChunks, CurrentCoords, CurrentText, Document.IndexYear, AcceptNoYear);
    return true;
}

void TDaterScanContext::Process(EDaterMode mode) {
    PrepareForScan();

    ui32 scancnt = NImpl::MAX_SCANS;
    ui32 chunkcnt = NImpl::MAX_CHUNKS_IN_ALL_SCANS;

    ScanUrl();

    if (ScanTextRange(Document.GetTitle(), TitleDates, true) && !UpdateLimits(scancnt, chunkcnt, CurrentChunks)) {
        return;
    }

    // segments
    for (NSegm::TSegmentSpans::const_iterator it = Document.SegmentSpans.begin(); it != Document.SegmentSpans.end(); ++it) {
        bool ismain = it->MainContentBackAdjoining || it->MainContentFrontAdjoining || it->InMainContentNews || it->HasMainHeaderNews;

        if (EqualToOneOf(mode, DM_MAIN_DATES, DM_MAIN_DATES_MAIN_RANGES) && !ismain) {
            continue;
        }

        NSegm::TRange seg = Document.GetBody().SelectBySpan(*it);
        CurrentRanges.clear();
        seg.SelectByBreak<NSegm::SET_PARABRK>(CurrentRanges);

        // paragraphs
        for (NSegm::TRanges::const_iterator rit = CurrentRanges.begin(); rit != CurrentRanges.end(); ++rit) {
            bool withranges = EqualToOneOf(mode, DM_ALL_DATES_ALL_RANGES, DM_MAIN_DATES_MAIN_RANGES)
                                    || DM_ALL_DATES_MAIN_RANGES == mode && ismain;

            if (ScanTextRange(*rit, TextDates, withranges) && !UpdateLimits(scancnt, chunkcnt, CurrentChunks)) {
                return;
            }
        }
    }
}

}
