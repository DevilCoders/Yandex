#include "numerator_utils.h"
#include <library/cpp/deprecated/dater_old/scanner/dater.h>
#include <kernel/segmentator/structs/structs.h>
#include <kernel/segnumerator/utils/event_storer.h>

using namespace NDater;
using namespace NSegm;

namespace NSegutils {

TDatePositions TDaterContext::GetAllDates() const {
    TDatePositions res;
    MakeDatePositions(res, UrlDates);

    const TDatePositions& txt = GetTextDates();
    res.insert(res.end(), txt.begin(), txt.end());
    return res;
}

TDatePositions TDaterContext::GetTextDates() const {
    TDatePositions res = TitleDates;
    res.insert(res.end(), BodyDates.begin(), BodyDates.end());
    return res;
}

void TDaterContext::Clear() {
    Ctx.Clear();
    UrlDates.clear();
    TitleDates.clear();
    BodyDates.clear();
    Stats.Clear();
    FilteredStats.Clear();
    TopDates.clear();
    Best = NDater::TDatePosition();
    TSegmentatorContext::Clear();
}

void TDaterContext::NumerateDoc() {
    TSegmentatorContext::NumerateDoc();

    if (UseDater2) {
        Ctx.Clear();

        Ctx.Document.Url = GetSegHandler()->GetOwnerInfo();
        Ctx.Document.SegmentSpans = GetSegHandler()->GetSegmentSpans();
        Ctx.Document.EventStorer = GetSegHandler()->GetStorer();

        const TRecognizer::TLanguages& langs = GetDoc().GuessedLanguages;

        Ctx.Document.MainLanguage = langs.size() < 1 ? LANG_UNK : langs[0].Language;
        Ctx.Document.AuxLanguage  = langs.size() < 2 ? LANG_UNK : langs[1].Language;
        Ctx.Document.IndexYear = GetDoc().Time.Date.Year;

        Ctx.Process(Mode);

        ND2::ConvertUrlDates(Ctx.UrlDates, UrlDates);
        ND2::ConvertTitleDates(Ctx.TitleDates, TitleDates);
        ND2::ConvertTextDates(Ctx.Document, Ctx.TextDates, BodyDates);

    } else {
        UrlDates = ScanUrl(GetDoc().Url.data(), GetDoc().Url.size());

        {
            TUtf16String s;
            TPosCoords p;
            TEventStorage& st = const_cast<TEventStorage&>(GetEvents(true));
            st.AsRange().ConcatenateText(s, p);
            TDateCoords td = ScanText(s.data(), s.size()); //, Doc.Time.Date.Year);
            MakeDatePositions(TitleDates, td, p, TDaterDate::FromTitle);
        }

        {
            TEventStorage& st = const_cast<TEventStorage&>(GetEvents(false));
            TRanges ranges;
            st.AsRange().SelectByBreak<SET_PARABRK>(ranges);
            TUtf16String s;
            TPosCoords p;
            for (TRanges::const_iterator it = ranges.begin(); it != ranges.end(); ++it) {
                s.clear();
                p.clear();
                it->ConcatenateText(s, p);

                TDateCoords td = ScanText(s.data(), s.size()); //, Doc.Time.Date.Year);
                MakeDatePositions(BodyDates, td, p);
            }

            MarkDatePositions(BodyDates, TSpan(), GetMainContentSpans(), GetSegmentSpans());
        }
    }

    {
        TDatePositions urldates;
        MakeDatePositions(urldates, UrlDates);
        TDatePositions alltext = GetTextDates();

        for (TDatePositions::const_iterator it = urldates.begin(); it != urldates.end(); ++it) {
            Stats.Add(*it);
        }

        for (TDatePositions::const_iterator it = alltext.begin(); it != alltext.end(); ++it) {
            Stats.Add(*it);
        }

        FindBestDate(Best, urldates, alltext, GetDoc().Time.Date, &TopDates);
        FilterUnreliableDates(urldates, alltext, GetDoc().Time.Date);

        for (TDatePositions::const_iterator it = urldates.begin(); it != urldates.end(); ++it) {
            FilteredStats.Add(*it);
        }

        for (TDatePositions::const_iterator it = alltext.begin(); it != alltext.end(); ++it) {
            FilteredStats.Add(*it);
        }
    }
}

}
