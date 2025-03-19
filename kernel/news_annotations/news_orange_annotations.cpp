#include "news_orange_annotations.h"

#include <library/cpp/deprecated/split/split_iterator.h>

static const char* datePrefix = "\tnews=date=";
static const char* titlePrefix = "title=";
static const char* urlLinkPrefix = "link_url=";
static const char* urlTitlePrefix = "link_title=";
static const char* snippetPrefix = "snippet=";
static const char* agencyRatePrefix = "agency_rate=";
static const char* countPrefix = "count=";
static const char* delimiter = "\07;";

TString TNewsOrangeAnnotation::Serialize() {
    TString result;
    result += datePrefix + ToString(Date.Seconds());

    result += delimiter;
    result += titlePrefix + Title;

    result += delimiter;
    result += snippetPrefix + Snippet;

    result += delimiter;
    result += agencyRatePrefix + ToString(AgencyRate);

    result += delimiter;
    result += countPrefix + Count;
    for (size_t i = 0; i < Urls.size(); i++) {
        result += delimiter;
        result += urlLinkPrefix;
        result += Urls[i].first;

        result += delimiter;
        result += urlTitlePrefix;
        result += Urls[i].second;
    }
    return result;
}

template<class TValue>
static TValue ProcessDataWithPrefix(const TStringBuf& data, const char* prefix, size_t prefixLen) {
    if (data.StartsWith(prefix)) {
        TStringBuf res = TStringBuf(&((data.data())[prefixLen]), data.length() - prefixLen);
        return FromString<TValue>(res);
    }

    ythrow yexception() << "Wrong prefixe's data: " << data;
}

static void CheckToken(const TStringBuf& token) {
    Y_ENSURE(!token.empty(), "Empty token");
}

void TNewsOrangeAnnotation::Parse(const TString& annotationData) {
    TSplitDelimiters delims(delimiter);
    TDelimitersSplit split(annotationData, delims);
    TDelimitersSplit::TIterator it = split.Iterator();

    // parse date
    TStringBuf rawData = it.NextTok();
    CheckToken(rawData);
    Date = TInstant::Seconds(ProcessDataWithPrefix<ui64>(rawData, datePrefix, strlen(datePrefix)));

    // parse title
    rawData = it.NextTok();
    CheckToken(rawData);
    Title = ProcessDataWithPrefix<TString>(rawData, titlePrefix, strlen(titlePrefix));

    // parse snippet
    rawData = it.NextTok();
    CheckToken(rawData);
    Snippet = ProcessDataWithPrefix<TString>(rawData, snippetPrefix, strlen(snippetPrefix));

    // parse AgencyRate
    rawData = it.NextTok();
    CheckToken(rawData);
    AgencyRate = ProcessDataWithPrefix<float>(rawData, agencyRatePrefix, strlen(agencyRatePrefix));

    // parse Count
    rawData = it.NextTok();
    CheckToken(rawData);
    Count = ProcessDataWithPrefix<size_t>(rawData, countPrefix, strlen(countPrefix));

    for (size_t i = 0; i < Count; ++i) {
        TStringBuf linkUrlData = it.NextTok();
        CheckToken(linkUrlData);
        TStringBuf linkUrl = ProcessDataWithPrefix<TString>(linkUrlData, urlLinkPrefix, strlen(urlLinkPrefix));

        TStringBuf linkTitleData = it.NextTok();
        CheckToken(linkUrlData);
        TStringBuf linkTitle = ProcessDataWithPrefix<TString>(linkTitleData, urlTitlePrefix, strlen(urlTitlePrefix));

        Urls.push_back(std::make_pair<TString, TString>(TString{linkUrl}, TString{linkTitle}));
    }
}

void TNewsOrangeAnnotation::Clear() {
    Date = TInstant();
    Title.clear();
    Snippet.clear();
    AgencyRate = 0.0;
    Count = 0;
    Urls.clear();
}

bool TNewsOrangeAnnotation::IsEmpty() {
    // Check only Ursl - its enought
    return (Urls.size() == 0);
}
