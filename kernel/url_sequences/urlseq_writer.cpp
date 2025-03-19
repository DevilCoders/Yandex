#include "urlseq_writer.h"
#include <ysite/yandex/common/prepattr.h>
#include <kernel/stringmatch_tracker/matchers/charmap.h>
#include <kernel/translit/translit.h>
#include <util/charset/wide.h>
#include <library/cpp/charset/recyr.hh>
#include <library/cpp/string_utils/quote/quote.h>
#include <util/generic/algorithm.h>

namespace NSequences {

TString TArrayBuilder::PrepareUrl(const TString& inurl,
                                 bool doTranslit,
                                 const TQueryRecognizer * const recognizer,
                                 const ELanguage defaultLanguage)
{
    TString url = PrepareURL(inurl);

    if (url.substr(0, 4) == "www.") {
        url = url.substr(4, url.size() - 4);
    }

    if (!doTranslit) {
        return Recode(CODES_WIN, CODES_YANDEX, url);
    }

    url = Recode(CODES_WIN, CODES_UTF8, url);

    try {
        ELanguage urlLanguage = defaultLanguage;
        if (!!recognizer) {
            urlLanguage = recognizer->RecognizeParsedQueryLanguage(UTF8ToWide(url), nullptr).GetMainLang();
        }

        url = TransliterateBySymbol(url, urlLanguage);
    } catch (...) {
        // shit happened with encoding/decoding
    }

    return Recode(CODES_UTF8, CODES_YANDEX, url);
}

void TArrayBuilder::AddUrl(const TString& inurl,
                           ui32 docId,
                           bool doTranslit,
                           const TQueryRecognizer * const recognizer,
                           const ELanguage defaultLanguage)
{
    TString url = PrepareUrl(inurl, doTranslit, recognizer, defaultLanguage);
    ui8 domainLen = 0;
    ui8 pathLen = 0;
    url = ShortenUrl(url, &domainLen, &pathLen);
    AddPreprocessedUrl(url, docId, domainLen, pathLen);
}

void TArrayBuilder::AddPreprocessedUrl(const TString& url, ui32 docId, ui8 domainLen, ui8 pathLen) {
    if (DocIdSet.find(docId) != DocIdSet.end())
        return;

    DocIdSet.insert(docId);
    if (MaxDocId < docId)
        MaxDocId = docId;

    DocUrls.push_back(TDocUrl(docId, url, domainLen, pathLen));
}

struct TCmpDocId {
    bool operator()(const TDocUrl& one, const TDocUrl& another) const {
        return one.DocId < another.DocId;
    }
};

struct TCmpDocUrl {
    bool operator()(const TDocUrl& one, const TDocUrl& another) const {
        if (one.Url == another.Url) {
            if (one.PrefixId == another.PrefixId) {
                return one.HopCount < another.HopCount;
            } else {
                return one.PrefixId < another.PrefixId;
            }
        } else
            return one.Url < another.Url;
    }
};

static size_t CalcMatchPrefix(const TString& one, const TString& another) {
    size_t pos = 0;
    while (pos < one.size() && pos < another.size() && one[pos] == another[pos])
        pos++;

    return pos;
}


size_t TArrayBuilder::EstimateShortenSize(size_t urlSize) {
    return Min<size_t>(urlSize, Max<ui8>());
}


void TArrayBuilder::CalcSegmentsLens(TStringBuf inurl, ui8& domainLen, ui8& pathLen) {
    //apply this function to a shortened url
    domainLen = 0;
    pathLen = 0;
    size_t n = inurl.size();
    size_t p = 0;
    while(p < n && inurl[p] != '/') {
        ++p;
    }
    domainLen = p;
    pathLen = n - p;
    if (pathLen)
        --pathLen;
}


size_t TArrayBuilder::ShortenUrl(TStringBuf inurl, size_t len, char* out) {
    size_t r = 0;
    bool inDomain = 1;
    for (size_t pos = 0; pos < len; ++pos) {
        if (IsLetter(inurl[pos])) {
            out[r] = inurl[pos];
            ++r;
        } else if (inurl[pos] == '/' && inDomain) {
            inDomain = 0;
            out[r] = inurl[pos];
            ++r;
        }
    }
    return r;
}

TString TArrayBuilder::ShortenUrl(const TString& inurl, ui8* domainLen, ui8* pathLen) {
    TString res;
    size_t n = TArrayBuilder::EstimateShortenSize(inurl.size());
    res.resize(n);
    size_t len = ShortenUrl(inurl, n, res.begin());
    res.resize(len);
    Y_ASSERT(domainLen && pathLen);
    CalcSegmentsLens(res, *domainLen, *pathLen);
    return res;
}

void TArrayBuilder::SortByDocId() {
    StableSort(DocUrls.begin(), DocUrls.end(), TCmpDocId());
}

void TArrayBuilder::WriteArray() {
    StableSort(DocUrls.begin(), DocUrls.end(), TCmpDocUrl());
    DocUrlById.assign(MaxDocId + 1, nullptr);

    for (size_t i = 0; i < DocUrls.size(); ++i)
        DocUrlById[DocUrls[i].DocId] = &DocUrls[i];

    for (size_t i = 1; i < DocUrls.size(); ++i) {
        size_t matches = CalcMatchPrefix(DocUrls[i].Url, DocUrls[i - 1].Url);

        if (!matches)
            continue;

        ui32 hopCandidateId = DocUrls[i - 1].DocId;

        while (DocUrlById[hopCandidateId]->PrefixId != (ui32)-1) {
            ui32 nextHopCandidateId = DocUrlById[hopCandidateId]->PrefixId;
            if (CalcMatchPrefix(DocUrls[i].Url, DocUrlById[nextHopCandidateId]->Url) < matches)
                break;

            hopCandidateId = nextHopCandidateId;
        }

        DocUrls[i].HopCount = DocUrlById[hopCandidateId]->HopCount + 1;
        DocUrls[i].PrefixId = hopCandidateId;
        DocUrls[i].PrefixLen = matches;

        for (ui32 nodeId = DocUrls[i].PrefixId; nodeId != (ui32)-1; nodeId =  DocUrlById[nodeId]->PrefixId) {
            DocUrlById[nodeId]->LeafCount++;
        }
    }

    for (size_t docId = 0; docId < DocUrlById.size(); ++docId) {
        if (!DocUrlById[docId]) {
            UrlsWriter.NewLine();

            continue;
        }

        TDocUrl &currDoc = *DocUrlById[docId];
        if (currDoc.LeafCount * currDoc.HopCount > (currDoc.DomainLen + currDoc.PathLen) * (size_t)3) {
            currDoc.PrefixLen = 0;
            currDoc.PrefixId = (ui32)-1;
        }

        char* pDocId = (char*)&currDoc.PrefixId;
        UrlsWriter.Write(pDocId[0]);
        UrlsWriter.Write(pDocId[1]);
        UrlsWriter.Write(pDocId[2]);
        UrlsWriter.Write(pDocId[3]);
        UrlsWriter.Write((char)currDoc.PrefixLen);
        UrlsWriter.Write(currDoc.DomainLen);
        UrlsWriter.Write(currDoc.PathLen);

        for (size_t pos = currDoc.PrefixLen; pos < currDoc.Url.size(); pos++)
            UrlsWriter.Write(currDoc.Url[pos]);

        if (docId < DocUrlById.size() - 1)
            UrlsWriter.NewLine();
    }

    UrlsWriter.Finish();
}

}
