#pragma once

#include <library/cpp/on_disk/2d_array/array2d.h>
#include <util/generic/vector.h>
#include <util/generic/string.h>
#include <util/system/filemap.h>
#include <kernel/alice/music_scenario/web_url_canonizer/lib/web_url_canonizer.h>
#include <kernel/indexdoc/omnidoc_fwd.h>
#include <kernel/doom/offroad_erf_wad/erf_io.h>
#include "urlseq_writer.h"


namespace NSequences {

struct TEntry {
    ui32 PrefixId = 0;
    ui8 PrefixLen = 0;
    ui8 DomainLen = 0;
    ui8 PathLen = 0;
};

class TArray : TNonCopyable {
    FileMapped2DArray<ui32, char> File;
    TVector<TString> Array;
    bool IsFile = false;

public:
    TArray() {
    }

    explicit TArray(const TString& name)
        : IsFile(true)
    {
        File.InitPrecharged(name.data());
    }

    explicit TArray(const TMemoryMap& mapping)
        : File(mapping)
        , IsFile(true)
    {
    }

    void Resize(size_t size) {
        Y_ASSERT(!IsFile);
        Array.resize(size);
    }

    const TString& operator [](size_t i) const {
        Y_ASSERT(!IsFile && i < Array.size());
        return Array[i];
    }

    size_t GetSize() const {
        if (Y_LIKELY(IsFile)) {
            return File.Size();
        }
        else {
            return Array.size();
        }
    }

    const char* GetBegin(size_t i) const {
        if (Y_LIKELY(IsFile)) {
            return File.GetBegin(i);
        }
        else {
            return Array[i].data();
        }
    }

    const char* GetEnd(size_t i) const {
        if (Y_LIKELY(IsFile)) {
            return File.GetEnd(i);
        }
        else {
            return Array[i].data() + Array[i].size();
        }
    }
};

struct TDocTextData {
    TString Data;
    ui8 DomainLen = 0;
    ui8 PathLen = 0;
    bool UrlHasHttps = 0;
    ui8 YandexMusicUrlType = 0;
};

class IOnlineUrlStorage {
public:
    using TAccessor = TOmniUrlAccessor;
    virtual TDocTextData GetData(ui32 docId, TOmniUrlAccessor* accessor, TBasesearchErfAccessor* erfAccessor) const = 0;
    virtual size_t GetSize() const = 0;
    virtual ~IOnlineUrlStorage() {}
};

class IOnlineTitleStorage {
public:
    using TAccessor = TOmniTitleAccessor;
    virtual TDocTextData GetData(ui32 docId, TOmniTitleAccessor* accessor, TBasesearchErfAccessor* erfAccessor) const = 0;
    virtual size_t GetSize() const = 0;
    virtual ~IOnlineTitleStorage() {}
};

template<class Storage>
class TRealTimeSomeStorage : public Storage {
public:
    TRealTimeSomeStorage(size_t maxDocs, bool transliterate)
        : UrlStorage(maxDocs)
        , Transliterate(transliterate)
    {
    }

    TDocTextData GetData(ui32 docId, typename Storage::TAccessor*, TBasesearchErfAccessor*) const override {
        Y_ASSERT(docId < UrlStorage.size());
        return UrlStorage[docId];
    }

    size_t GetSize() const override {
        return UrlStorage.size();
    }

    void SetUrlByDocId(ui32 docId, const TString& clearUrl) {
        TDocTextData& urlInfo = UrlStorage[docId];
        TString prepared = NSequences::TArrayBuilder::PrepareUrl(clearUrl, Transliterate);
        urlInfo.DomainLen = 0;
        urlInfo.PathLen = 0;
        urlInfo.Data = TArrayBuilder::ShortenUrl(prepared, &urlInfo.DomainLen, &urlInfo.PathLen);
        urlInfo.UrlHasHttps = clearUrl.StartsWith("https:");
        if (clearUrl.Contains("music.yandex")) {
            NAlice::NMusic::EMusicUrlType musicUrlType = NAlice::NMusic::ParseMusicDataFromDocUrl(clearUrl).Type;
            urlInfo.YandexMusicUrlType = static_cast<ui8>(musicUrlType);
        }
    }
private:
    TVector<TDocTextData> UrlStorage;
    const bool Transliterate;
};

using TRealTimeUrlStorage = TRealTimeSomeStorage<IOnlineUrlStorage>;
using TRealTimeTitleStorage = TRealTimeSomeStorage<IOnlineTitleStorage>;

}
