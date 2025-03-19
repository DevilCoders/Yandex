#pragma once

#include "url_to_positions.h"

#include <yweb/robot/dbscheeme/baserecords.h> // TUrlRec

#include <library/cpp/containers/comptrie/comptrie.h>

#include <util/generic/vector.h>

class TBlob;

typedef TVector<size_t> TIndexPositions;

class IHasUrl {
public:
    virtual bool HasUrl(const TString& url, const TIndexPositions** positions) const = 0;
    virtual size_t GetSize() const = 0;
    virtual TString GetUrl(size_t index) const = 0;
    virtual ~IHasUrl() {};
};

IHasUrl* CreateHashHasUrl(const TUrls& urls, bool toLower);
IHasUrl* CreateHashHasUrl(const TString& urlsFilename, bool toLower, char sep = '\t', ui32 field = 0);
IHasUrl* CreateMappedHasUrl(const TBlob& data, bool toLower);

typedef TVector<TString> TUrls;
typedef TVector<ui32> TDocIds;

class IUrlRecReciever {
public:
    IUrlRecReciever()
    {
    }

    virtual ~IUrlRecReciever()
    {
    }

    virtual void Do(const TUrlRec* rec, size_t pos) = 0;
};

void Urls2Reciever(const TString& index, const IHasUrl& urls, IUrlRecReciever& recieve);

void Urls2DocIds(const TString& index, const IHasUrl& url, TDocIds* result);

void Urls2DocIds(const TString& index, const TUrls& urls, TDocIds* result, bool toLower = false);

void PrepareMappedHasUrl(const TUrls& urls, IOutputStream& outStream, bool toLower = false);
void Urls2DocIds(const TString& index, const TBlob& urlTrie, TDocIds* result, bool toLower = false);

// 1) uses external sort (need a permission to create temporary files)
// 2) much faster than Urls2DocIds for big amounts of urls (say, 100K+)
void Urls2DocIdsExtSort(const TString& index, const TUrls& urls, TDocIds* result, bool toLower = false);

typedef TVector<TUrls> TUrlsVector;
void Urls2Groups(const TString& index, const TUrl2PositionsReader& urls, TUrlsVector* result);
