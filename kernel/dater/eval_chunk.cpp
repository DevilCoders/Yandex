#include "scanner.h"


namespace ND2 {
namespace NImpl {

static void SetDateDMY(TChunk& ch, ui32 didx, ui32 midx, ui32 yidx) {
    ch[didx].RemoveMeaning<SS_DATE>();
    ch[midx].RemoveMeaning<SS_DATE>();
    ch[yidx].RemoveMeaning<SS_DATE>();
    ch[didx].AddMeaning<SS_DAY>();
    ch[midx].AddMeaning<SS_MONTH>();
    ch[yidx].AddMeaning<SS_YEAR>();
}

static void SetDateMY(TChunk& ch, ui32 midx, ui32 yidx) {
    ch[midx].RemoveMeaning<SS_DATE>();
    ch[yidx].RemoveMeaning<SS_DATE>();
    ch[midx].SetMeaning<SS_MONTH>();
    ch[yidx].SetMeaning<SS_YEAR>();
}


static void SetDateDM(TChunk& ch, ui32 didx, ui32 midx) {
    ch[didx].RemoveMeaning<SS_DATE>();
    ch[midx].RemoveMeaning<SS_DATE>();
    ch[didx].SetMeaning<SS_DAY>();
    ch[midx].SetMeaning<SS_MONTH>();
}

static bool TryDM(TChunk& ch) {
    ui32 didx = ch.FindFirstOf<SS_DAY>();
    ui32 midx = ch.FindLastOf<SS_MONTH>(didx);

    if (EqualToOneOf(ui32(TChunk::NoSpan), didx, midx))
        return false;

    SetDateDM(ch, didx, midx);
    return true;
}

static bool TryMD(TChunk& ch) {
    ui32 midx = ch.FindFirstOf<SS_MONTH>();
    ui32 didx = ch.FindLastOf<SS_DAY>(midx);

    if (EqualToOneOf(ui32(TChunk::NoSpan), didx, midx))
        return false;

    SetDateDM(ch, didx, midx);
    return true;
}

static bool TryDMY(TChunk& ch, const TValidDateProfile& pr) {
    ui32 yidx = ch.FindLastOf<SS_YEAR>();
    ui32 didx = ch.FindFirstOf<SS_DAY>(yidx);
    ui32 midx = ch.FindFirstOf<SS_MONTH>(yidx, didx);

    if (EqualToOneOf(ui32(TChunk::NoSpan), yidx, midx) || pr.MinDayCount && TChunk::NoSpan == didx)
        return false;

    if (TChunk::NoSpan != didx)
        SetDateDMY(ch, didx, midx, yidx);
    else
        SetDateMY(ch, midx, yidx);

    return true;
}

static bool TryMDY(TChunk& ch) {
    ui32 yidx = ch.FindLastOf<SS_YEAR>();
    ui32 midx = ch.FindFirstOf<SS_MONTH>(yidx);
    ui32 didx = ch.FindFirstOf<SS_DAY>(yidx, midx);
    if (EqualToOneOf(ui32(TChunk::NoSpan), yidx, midx, didx))
        return false;

    SetDateDMY(ch, didx, midx, yidx);
    return true;
}

static bool TryYMD(TChunk& ch, const TValidDateProfile& pr) {
    ui32 yidx = ch.FindFirstOf<SS_YEAR>();
    ui32 didx = ch.FindLastOf<SS_DAY>(yidx);
    ui32 midx = ch.FindFirstOf<SS_MONTH>(didx, yidx);

    if (EqualToOneOf(ui32(TChunk::NoSpan), yidx, midx) || pr.MinDayCount && TChunk::NoSpan == didx)
        return false;

    if (TChunk::NoSpan != didx)
        SetDateDMY(ch, didx, midx, yidx);
    else
        SetDateMY(ch, midx, yidx);

    return true;
}

bool SetJunk(TChunk& ch) {
    ch.Reset(CS_JUNK, SS_JUNK);
    return false;
}

static bool FixYear2(TChunk& ch, ui32 refyear) {
    const ui32 sz = ch.SpanCount();
    for (ui32 i = 0; i < sz; ++i) {
        if (ch[i].HasMeaning<SS_YEAR>() && ch[i].Size() == 2) {
            ch[i].Value = TValidDateProfile::Year2ToYear4(ch[i].Value, refyear);
            break;
        }
    }

    return true;
}

bool DisambiguateChunk(TChunk& ch, ECountryType ct, ui32 idxyear, bool frompath) {
    if (!ch.IsMeaningfull())
        return false;

    const ui32 sz = ch.SpanCount();

    for (ui32 i = 0; i < sz; ++i) {
        const TChunkSpan& sp = ch.GetSpan(i);

        if (sp.HasMeaning<SS_JUNK>()) {
            return SetJunk(ch);
        }
    }

    ui32 daycount = 0;
    ui32 monthcount = 0;
    ui32 yearcount = 0;
    ui32 dspcount = 0;

    const TValidDateProfile pr(ch.PatternType);
    ch.Meaning |= pr.IsDateRange ? CS_DATE_RANGE : pr.IsDate ? CS_DATE : pr.IsTime ? CS_TIME : 0;

    for (ui32 i = 0; i < sz; ++i) {
        TChunkSpan& sp = ch.GetSpan(i);

        if (sp.HasMeaning<SS_TIME>()) {
            sp.RemoveMeaning<SS_DATE>();
            ch.Meaning |= CS_TIME;
            continue;
        }

        if (sp.HasSomeOfMeanings<SS_DATE>() && sp.HasMeaning<SS_NUM>()) {
            if (!EqualToOneOf(sp.Size(), size_t(1), size_t(2), size_t(4))) {
                return SetJunk(ch);
            }
        }

        if (sp.HasSomeOfMeanings<SS_DATE>() && sp.HasMeaning<SS_NUM>()) {
            sp.Value = Atoi(sp);

            if (sp.HasMeaning<SS_DAY>() && (sp.Value > 31 || !sp.Value || sp.Size() > 2))
                sp.RemoveMeaning<SS_DAY>();

            if (sp.HasMeaning<SS_MONTH>() && (sp.Value > 12 || !sp.Value || sp.Size() > 2))
                sp.RemoveMeaning<SS_MONTH>();

            if (sp.HasMeaning<SS_YEAR>()) {
                ui32 val;

                if (sp.Size() == 4) {
                    sp.RemoveMeaning<SS_DAY | SS_MONTH>();
                    val = sp.Value;
                } else {
                    val = TValidDateProfile::Year2ToYear4(sp.Value, idxyear);
                }

                if (!pr.GoodYear(val, idxyear)) {
                    if (sp.Size() == 2) {
                        sp.RemoveMeaning<SS_YEAR>();
                    } else {
                        return SetJunk(ch);
                    }
                }
            }
        }

        if (sp.HasMeaning<SS_DAY>()) {
            ++daycount;
        }

        if (sp.HasMeaning<SS_MONTH>()) {
            ++monthcount;
        }

        if (sp.HasMeaning<SS_YEAR>()) {
            ++yearcount;
        }

        if (sp.HasSomeOfMeanings<SS_DATE>())
            ++dspcount;
    }

    if (!pr.GoodPattern(dspcount, daycount, monthcount, yearcount)) {
        return SetJunk(ch);
    }

    // ambiguous patterns
    if (pr.IsAmbiguous) {
        if (frompath && TryYMD(ch, pr))
            return FixYear2(ch, idxyear);

        if (pr.MinYearCount) {
            if (    CT_MDY == ct && (TryMDY(ch) || TryDMY(ch, pr) || TryYMD(ch, pr))
                 || CT_DMY == ct && (TryDMY(ch, pr) || TryYMD(ch, pr) || TryMDY(ch)))
                return FixYear2(ch, idxyear);
        } else {
            if (    CT_MDY == ct && (TryMD(ch) || TryDM(ch))
                 || CT_DMY == ct && (TryDM(ch) || TryMD(ch)))
                return true;
        }

        return SetJunk(ch);
    }

    return FixYear2(ch, idxyear);
}

}
}
