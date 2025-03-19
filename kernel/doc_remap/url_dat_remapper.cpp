#include "remap_reader.h"
#include "url_to_positions.h"
#include "url_dat_remapper.h"

#include <library/cpp/containers/comptrie/chunked_helpers_trie.h>
#include <library/cpp/microbdb/safeopen.h>
#include <library/cpp/string_utils/old_url_normalize/url.h>

#include <util/generic/hash.h>
#include <util/generic/strbuf.h>
#include <library/cpp/sorter/sorter.h>
#include <util/stream/file.h>
#include <util/string/cast.h>
#include <util/string/split.h>
#include <util/ysaveload.h>

class THashHasUrl : public IHasUrl {
    TUrls Urls;
    typedef THashMap<TString, TIndexPositions> TUrl2Positions;
    TUrl2Positions Url2Positions;
    size_t Size;
    bool ToLower;

public:
    THashHasUrl(const TUrls& urls, bool toLower)
        : Urls(urls)
        , ToLower(toLower)
    {
        for (size_t i = 0; i < Urls.size(); ++i) {
            TString url = NormalizeUrl(Urls[i]);

            if (ToLower)
                url.to_lower();

            Url2Positions[url].push_back(i);
        }
        Size = Urls.size();
    }

    THashHasUrl(const TString& urlsFilename, char sep, ui32 field, bool toLower)
        : ToLower(toLower)
    {
        TFileInput in(urlsFilename);
        size_t i = 0;
        TString line;
        TVector<TStringBuf> parts;
        while (in.ReadLine(line)) {
            parts.clear();
            StringSplitter(line).Split(sep).AddTo(&parts);
            if (parts.size() <= field)
                continue;

            Urls.push_back(ToString(parts[field]));
            TString url = NormalizeUrl(Urls.back());

            if (ToLower)
                url.to_lower();

            Url2Positions[url].push_back(i);
            ++i;
        }
        Size = Urls.size();
    }

    bool HasUrl(const TString& url, const TIndexPositions** positions) const override {
        TString nurl = NormalizeUrl(url);

        if (ToLower)
            nurl.to_lower();

        TUrl2Positions::const_iterator toUrl = Url2Positions.find(nurl);
        if (toUrl != Url2Positions.end()) {
            *positions = &toUrl->second;
            return true;
        } else {
            return false;
        }
    }

    size_t GetSize() const override {
        return Size;
    }

    TString GetUrl(size_t index) const override {
        return Urls[index];
    }
};

void PrepareMappedHasUrl(const TUrls& urls, IOutputStream& outStream, bool toLower) {
    THashHasUrl hashHas(urls, toLower);

    TChunkedDataWriter dataWriter(outStream);

    TTrieSortedMapWriter<ui64> setWriter;
    TYVectorWriter<ui32> positionsWriter;
    for (size_t i = 0; i < urls.size(); ++i) {
        if (!urls[i].empty()) {
            const TIndexPositions* positions = nullptr;
            bool result = hashHas.HasUrl(urls[i], &positions);
            if (!result)
                ythrow yexception() << "positions for url not found";
            ui64 pos = (((ui64)positionsWriter.Size()) << 32) + positions->size();

            TString nurl = NormalizeUrl(urls[i]);

            if (toLower)
                nurl.to_lower();

            setWriter.Add(nurl.data(), pos);
            for (size_t j = 0; j < positions->size(); ++j)
                positionsWriter.PushBack((ui32)(*positions)[j]);
        }
    }
    WriteBlock(dataWriter, setWriter);
    WriteBlock(dataWriter, positionsWriter);

    TSingleValueWriter<ui64> sizeWriter(urls.size());
    WriteBlock(dataWriter, sizeWriter);

    TStringsVectorWriter urlsVector;
    for (size_t i = 0; i < urls.size(); ++i)
        urlsVector.PushBack(urls[i]);
    WriteBlock(dataWriter, urlsVector);

    dataWriter.WriteFooter();
}

class TMappededHasUrl : public IHasUrl {
private:
    bool ToLower;
    TTrieMap<ui64> Urls;
    TYVector<ui32> Positions;
    TSingleValue<ui64> Size;
    TStringsVector UrlsVector;
    mutable TIndexPositions TempPositions;

public:
    TMappededHasUrl(const TBlob& data, bool toLower)
        : ToLower(toLower)
        , Urls(GetBlock(data, 0))
        , Positions(GetBlock(data, 1))
        , Size(GetBlock(data, 2))
        , UrlsVector(GetBlock(data, 3))
    {
    }

    bool HasUrl(const TString& url, const TIndexPositions** positions) const override {
        TString nurl = NormalizeUrl(url);

        if (ToLower)
            nurl.to_lower();

        ui64 pos;
        if (Urls.Get(nurl.data(), &pos)) {
            ui32 size = pos & 0xFFFFFFFF;
            ui32 offset = pos >> 32;
            Y_ASSERT(offset + size <= Positions.GetSize());
            TempPositions.clear();
            for (size_t i = 0; i < size; ++i) {
                Y_ASSERT(Positions.At(offset + i) < Size.Get());
                TempPositions.push_back(Positions.At(offset + i));
            }
            *positions = &TempPositions;
            return true;
        } else {
            return false;
        }
    }

    size_t GetSize() const override {
        return (size_t)Size.Get();
    }

    TString GetUrl(size_t index) const override {
        return UrlsVector.Get(index);
    }
};

IHasUrl* CreateHashHasUrl(const TUrls& urls, bool toLower) {
    return new THashHasUrl(urls, toLower);
}

IHasUrl* CreateHashHasUrl(const TString& urlsFilename, bool toLower, char sep, ui32 field) {
    return new THashHasUrl(urlsFilename, sep, field, toLower);
}

IHasUrl* CreateMappedHasUrl(const TBlob& data, bool toLower) {
    return new TMappededHasUrl(data, toLower);
}

void Urls2Reciever(const TString& index, const IHasUrl& urls, IUrlRecReciever& recieve) {
    const TString fInput = index + "/url.dat";
    TInDatFile<TUrlRec> dbUrls(fInput.data(), 20 << 20, 0);
    dbUrls.Open(fInput.data());

    const TIndexPositions* positions;
    while (const TUrlRec* url = dbUrls.Next()) {
        if (urls.HasUrl(url->Name, &positions))
            for (size_t i = 0; i < positions->size(); ++i)
                recieve.Do(url, (*positions)[i]);
    }
}

class TGetDocIds: public IUrlRecReciever {
public:
    TGetDocIds(TDocIds* result, size_t size)
        : Result(result)
    {
        Result->clear();
        Result->resize(size, (ui32)-1);
    }

    void Do(const TUrlRec* rec, size_t pos) override {
        (*Result)[pos] = rec->DocId;
    }

private:
    TDocIds* Result;
};

void Urls2DocIds(const TString& index, const IHasUrl& urls, TDocIds* result) {
    TGetDocIds getDocIds(result, urls.GetSize());
    Urls2Reciever(index, urls, getDocIds);
}

void Urls2DocIds(const TString& index, const TUrls& urls, TDocIds* result, bool toLower) {
    THashHasUrl hashHas(urls, toLower);
    Urls2DocIds(index, hashHas, result);
}

void Urls2DocIds(const TString& index, const TBlob& urlTrie, TDocIds* result, bool toLower) {
    TMappededHasUrl mappedHas(urlTrie, toLower);
    Urls2DocIds(index, mappedHas, result);
}

struct TSortableUrlPos {
    char Url[URL_MAX];
    size_t Position;
    ui32 DocId;

    TSortableUrlPos() {
    }

    TSortableUrlPos(const TString &url, ui32 docId, size_t position) {
        strncpy(Url, url.data(), URL_MAX);
        Position = position;
        DocId = docId;
    }

    bool operator<(const TSortableUrlPos & rhs) const {
        int cmp = strcmp(Url, rhs.Url);
        if (cmp)
            return cmp < 0;
        if (Position != rhs.Position)
            return Position < rhs.Position;
        return DocId < rhs.DocId;
    }
};

void Urls2DocIdsExtSort(const TString& index, const TUrls& urls, TDocIds* result, bool toLower) {
    NSorter::TSorter<TSortableUrlPos> sorter(1024*1024*1024/sizeof(TSortableUrlPos));
    const TString fInput = index + "/url.dat";
    TInDatFile<TUrlRec> dbUrls(fInput.data(), 1);
    dbUrls.Open(fInput.data());
    while (const TUrlRec* url = dbUrls.Next()) {
        TString nurl = NormalizeUrl(url->Name);
        if (toLower)
            nurl.to_lower();
        sorter.PushBack(TSortableUrlPos(nurl, url->DocId, 0));
    }
    const ui32 NO_DOCID = (ui32)-1;
    for (size_t n = 0; n < urls.size(); ++n) {
        TString nurl = NormalizeUrl(urls[n]);
        if (toLower)
            nurl.to_lower();
        sorter.PushBack(TSortableUrlPos(nurl, NO_DOCID, n + 1));
    }
    NSorter::TIterator<TSortableUrlPos> it;
    sorter.Close(it);
    ui32 docId = NO_DOCID;
    TString lastUrl;
    while (!it.Finished()) {
        if (it->Url != lastUrl) {
            docId = it->DocId;
            lastUrl = it->Url;
        }
        if (it->Position > 0 && docId != NO_DOCID)
            (*result)[it->Position - 1] = docId;
        ++it;
    }
}

void Urls2Groups(const TString& index, const TUrl2PositionsReader& url2Positions, TUrlsVector* result) {
    result->resize(url2Positions.GetNUrls());

    typedef TVector<size_t> TIndexes;
    typedef THashMap<ui32, TIndexes> TDocId2Indexes;
    TDocId2Indexes doc2indexes;

    TString fInput = index + "/url.dat";
    TInDatFile<TUrlRec> dbUrls(fInput.data(), 1);
    dbUrls.Open(fInput.data());
    while (const TUrlRec* url = dbUrls.Next()) {
        const TString normalized = NormalizeUrl(url->Name);
        TPositions positions;
        if (url2Positions.Get(normalized, &positions)) {
            for (size_t i = 0; i < positions.size(); ++i)
                doc2indexes[url->DocId].push_back(positions[i]);
        }
    }

    dbUrls.Close();
    dbUrls.Open(fInput.data());

    while (const TUrlRec* url = dbUrls.Next()) {
        TDocId2Indexes::const_iterator toDocId = doc2indexes.find(url->DocId);
        if (toDocId != doc2indexes.end())
            for (size_t i = 0; i < toDocId->second.size(); ++i)
                (*result)[toDocId->second[i]].push_back(url->Name);
    }
}
