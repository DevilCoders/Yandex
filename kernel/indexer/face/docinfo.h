#pragma once

#include <util/generic/map.h>
#include <util/generic/string.h>
#include <kernel/tarc/iface/fulldoc.h>

struct TRangeInfo {
    ui32 BufferOffset;
    ui32 BufferSize;
    i32  ContentRangeStart;
    i32  ContentRangeEnd;
    i32  ContentRangeEntityLength;
};

struct TDocInfoEx {
    TFullArchiveDocHeader *DocHeader;
    const char* DocText;
    ui32 DocSize;
    ui32 DocId;
    ui32 UrlFlags;
    ui32 HostId;
    ui8 Hops;
    time_t ModTime;
    const char *ConvText;
    ui32 ConvSize;
    const char *ExtLinkDates;
    ui32 ExtLinkDatesSize;
    i32 ContentLength;
    TRangeInfo *Ranges;
    ui32 RangesSize;
    TString FullUrl;
    const char* AnchorData;
    ui32 AnchorSize;
    ui64 FeedId;
    ui8 IsLogical;
    ui8 IsPPBHost;
    ui8 IsMobileUrl;
    ui8 NeverReachableFromMorda;
    ui8 IsTurboEcom;
    ui8 IsTopClickedCommHost;
    ui8 IsYANHost;
    ui8 IsFilterExp1;
    ui8 IsFilterExp2;
    ui8 IsFilterExp3;
    ui8 IsFilterExp4;
    ui8 IsFilterExp5;
    TVector<ui32> ItdItpImagesMatchedFilters;
    bool FromTurboFeed;
    bool TurboFeedKill;
    bool SiteRecommendationKill;
    TVector<TString> TurboFeedTags;
    TVector<TString> SiteRecommendationTags;
    TString ZenSearchGroupingAttribute;

    TVector<TString> UrlTransliterationData;

    TVector<bool> YabsFlags;

    TDocInfoEx() {
        Clear();
    }

    void Clear() {
        DocHeader = nullptr;
        DocText = nullptr;
        DocSize = 0;
        DocId = 0; // it's valid value!
        UrlFlags = 0;
        HostId = 0;
        Hops = 0;
        ModTime = 0;
        ConvText = nullptr;
        ConvSize = 0;
        ExtLinkDates = nullptr;
        ExtLinkDatesSize = 0;
        ContentLength = 0;
        Ranges = nullptr;
        RangesSize = 0;
        FullUrl.clear();
        DocId = ui32(-1);
        ContentLength = -1;
        AnchorData = nullptr;
        AnchorSize = 0;
        FeedId = 0;
        IsLogical = 0;
        IsPPBHost = 0;
        IsMobileUrl = 0;
        NeverReachableFromMorda = 0;
        IsTurboEcom = 0;
        IsTopClickedCommHost = 0;
        IsYANHost = 0;
        IsFilterExp1 = 0;
        IsFilterExp2 = 0;
        IsFilterExp3 = 0;
        IsFilterExp4 = 0;
        IsFilterExp5 = 0;
        ItdItpImagesMatchedFilters.clear();
        FromTurboFeed = false;
        TurboFeedKill = false;
        SiteRecommendationKill = false;
        TurboFeedTags.clear();
        SiteRecommendationTags.clear();
        ZenSearchGroupingAttribute.clear();
        YabsFlags.clear();
    }
};
