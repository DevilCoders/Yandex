#pragma once

#include <kernel/segmentator/structs/spans.h>
#include <kernel/segmentator/structs/structs.h>

#include <kernel/segnumerator/utils/event_storer.h>
#include <library/cpp/unicode/folding/fold.h>

#include <util/draft/datetime.h>
#include <util/generic/utility.h>

namespace ND2 {

enum ECountryType {
    CT_DMY = 0,
    CT_MDY = 1,
};

struct TDaterDocumentContext {
    ELanguage MainLanguage;
    ELanguage AuxLanguage;

    ECountryType CountryType;

    ui32 IndexYear;

    struct TFlags {
        ui64 UrlHasDownloadPattern : 1;
    } Flags;

    NSegm::TUrlInfo Url;

    NSegm::TEventStorer EventStorer;

    NSegm::TParagraphSpans ParagraphSpans;
    NSegm::THeaderSpans HeaderSpans;
    NSegm::TSegmentSpans SegmentSpans;
    NSegm::TMainHeaderSpans MainHeaderSpans;
    NSegm::TMainContentSpans MainContentSpans;
    NSegm::TArticleSpans ArticleSpans;

    TDaterDocumentContext()
        : IndexYear(NDatetime::TSimpleTM::CurrentUTC().RealYear())
    {
        Clear();
    }

    NSegm::TRange GetTitle() {
        return EventStorer.GetStorage(true).AsRange();
    }

    NSegm::TRange GetBody() {
        return EventStorer.GetStorage(false).AsRange();
    }

    void Clear() {
        MainLanguage = LANG_UNK;
        AuxLanguage = LANG_UNK;
        CountryType = CT_DMY;
        Zero(Flags);
        Url.Clear();
        EventStorer.Clear();

        HeaderSpans.clear();
        SegmentSpans.clear();
        MainHeaderSpans.clear();
        MainContentSpans.clear();
        ArticleSpans.clear();
    }
};

}


