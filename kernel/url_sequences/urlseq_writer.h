#pragma once

#include <dict/recognize/queryrec/queryrecognizer.h>
#include <library/cpp/on_disk/2d_array/array2d_writer.h>

#include <library/cpp/charset/ci_string.h>
#include <util/generic/hash_set.h>
#include <util/generic/string.h>

namespace NSequences {

struct TDocUrl {
    ui32 DocId;
    TCiString Url;
    size_t HopCount;
    size_t LeafCount;
    ui32 PrefixId;
    ui8 PrefixLen;
    ui8 DomainLen;
    ui8 PathLen;

    TDocUrl(ui32 docId, TCiString url, ui8 domainLen, ui8 pathLen)
        : DocId(docId)
        , Url(url)
        , HopCount(0)
        , LeafCount(0)
        , PrefixId((ui32)-1)
        , PrefixLen(0)
        , DomainLen(domainLen)
        , PathLen(pathLen)
    {
    }
};

class TArrayBuilder {
private:
    TVector<TDocUrl> DocUrls;
    TVector<TDocUrl*> DocUrlById;
    THashSet<ui32> DocIdSet;

    TFile2DArrayWriter<ui32, char> UrlsWriter;

    ui32 MaxDocId;

public:
    TArrayBuilder(const TString& outputFileName)
        : UrlsWriter(outputFileName.data())
        , MaxDocId(0)
    {
    }

    void AddUrl(const TString& inurl,
                ui32 docId,
                bool doTranslit = false,
                const TQueryRecognizer * const recognizer = nullptr,
                const ELanguage defaultLanguage = LANG_RUS);
    void AddPreprocessedUrl(const TString& url, ui32 docId, ui8 domainLen, ui8 pathLen);

    void SortByDocId();
    void WriteArray();

    static size_t EstimateShortenSize(size_t urlSize);

    static void CalcSegmentsLens(TStringBuf inurl, ui8& domainLen, ui8& pathLen);

    static size_t ShortenUrl(TStringBuf inurl, size_t len, char* outbuf);
    static TString ShortenUrl(const TString& inurl, ui8* domainLen, ui8* pathLen);
    static TString PrepareUrl(const TString& inurl,
                             bool doTranslit = false,
                             const TQueryRecognizer * const recognizer = nullptr,
                             const ELanguage defaultLanguage = LANG_RUS);
};

} // namespace NSequences
