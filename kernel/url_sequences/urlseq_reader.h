#pragma once
#include "storage.h"

#include <library/cpp/langs/langs.h>

#include <util/generic/vector.h>
#include <util/generic/yexception.h>
#include <util/generic/singleton.h>

namespace NSequences {

template<typename T>
void GetReaderUrl(T* reader, TVector<char>& result) {
    result.clear();
    result.reserve(reader->GetDomainLen() + reader->GetPathLen() + 1);

    bool hasSlash = false;
    for (char c; c = reader->GetPrevCharacterFromUrl();) {
        result.push_back(c);
        if (Y_UNLIKELY(c == '/'))
            hasSlash = true;
    }

    const size_t expectedLen = size_t(reader->GetDomainLen()) + reader->GetPathLen() + (hasSlash ? 1 : 0);
    if (result.size() != expectedLen) {
        result.clear();
        return;
    }

    // See SEARCH-1703
    Y_ASSERT(result.size() == (size_t)reader->GetDomainLen() + (size_t)reader->GetPathLen() + hasSlash);

    std::reverse(result.begin(), result.end());
}

class TReaderBase : public TNonCopyable {
protected:
    ui8 DocDomainLen = 0;
    ui8 DocPathLen = 0;
    bool HasHttps = 0;
    ui8 YandexMusicUrlType = 0;
    ui32 CurrId = 0;

public:
    TReaderBase() = default;

    ui8 GetDomainLen() const {
        return DocDomainLen;
    }

    ui8 GetPathLen() const {
        return DocPathLen;
    }

    bool GetHasHttps() const {
        return HasHttps;
    }

    ui8 GetYandexMusicUrlType() const {
        return YandexMusicUrlType;
    }

    virtual char GetPrevCharacterFromUrl() = 0;
    virtual ~TReaderBase() = default;
};

class TReader : public TReaderBase {
private:
    const TArray& Array;

    const char* CurrBegin = nullptr;
    size_t CurrSize = 0;
    const char* CurrPos = nullptr;
    TEntry* CurrDoc = nullptr;

public:
    TReader(const TArray& array)
        : Array(array)
    {
    }

    bool CheckDoc(ui32 docId) const;
    void InitDoc(ui32 docId);

    char GetPrevCharacterFromUrl() override;

    TString GetUrl() {
        TVector<char> tmp;
        GetReaderUrl(this, tmp);
        return TString(tmp.begin(), tmp.end());
    }

    size_t GetCurrPos() const;
    const TEntry* GetCurrDoc() const;
};

template<class Storage>
class TRealTimeReader : public TReaderBase {
public:
    using IStorage = Storage;

    TRealTimeReader(const IStorage& urlStorage, typename IStorage::TAccessor* accessor, TBasesearchErfAccessor* erfAccessor)
        : CurrPos(0)
        , UrlStorage(urlStorage)
        , Accessor_(accessor)
        , ErfAccessor_(erfAccessor)
    {
    }

    void InitDoc(ui32 docId) {
        CurrId = docId;
        NSequences::TDocTextData docTextData = UrlStorage.GetData(docId, Accessor_, ErfAccessor_);
        CurrUrl = docTextData.Data;
        DocDomainLen = docTextData.DomainLen;
        DocPathLen = docTextData.PathLen;
        HasHttps = docTextData.UrlHasHttps;
        YandexMusicUrlType = docTextData.YandexMusicUrlType;
        CurrPos = CurrUrl.size();
    }

    size_t GetCurrPos() const {
        return CurrPos;
    }

    bool CheckDoc(ui32 docId) const {
        return (docId < UrlStorage.GetSize());
    }

    TString GetUrl() const {
        return CurrUrl;
    }

    char GetPrevCharacterFromUrl() {
        if (CurrPos == 0) {
            return 0;
        }
        --CurrPos;
        return CurrUrl[CurrPos];
    }
private:
    TString CurrUrl;
    size_t CurrPos;
    const IStorage& UrlStorage;
    typename IStorage::TAccessor* Accessor_;
    TBasesearchErfAccessor* ErfAccessor_;
};


inline bool TReader::CheckDoc(ui32 docId) const {
    Y_ENSURE(Array.GetBegin(docId) <= Array.GetEnd(docId), "Invalid sequence for docId " << docId << ". ");
    return Array.GetBegin(docId) < Array.GetEnd(docId);
}

inline void TReader::InitDoc(ui32 docId) {
    CurrId = docId;
    CurrBegin = Array.GetBegin(CurrId) + 7;

    Y_ENSURE(CurrBegin <= Array.GetEnd(CurrId), "Invalid or absent sequence for docId " << CurrId << ". ");

    CurrSize = Array.GetEnd(CurrId) - CurrBegin;
    CurrDoc = (TEntry*)Array.GetBegin(CurrId);
    CurrPos = CurrBegin + CurrSize;
    DocDomainLen = CurrDoc->DomainLen;
    DocPathLen = CurrDoc->PathLen;
}

inline size_t TReader::GetCurrPos() const {
    return CurrDoc->PrefixLen + CurrPos - CurrBegin;
}

inline const TEntry* TReader::GetCurrDoc() const {
    return CurrDoc;
}

inline char TReader::GetPrevCharacterFromUrl() {
    if (CurrPos != CurrBegin) {
        CurrPos--;

        return *CurrPos;
    }
    //Hop to next prefix
    CurrId = CurrDoc->PrefixId;
    const char* begin = Array.GetBegin(CurrId);
    const char* end = Array.GetEnd(CurrId);

    CurrBegin = begin + 7;
    if (CurrBegin > end) {
        return 0;
    }

    CurrSize = end - CurrBegin;
    ui8 oldPrefixLen = CurrDoc->PrefixLen;
    CurrDoc = (TEntry*)begin;
    CurrPos = CurrBegin + oldPrefixLen - CurrDoc->PrefixLen - 1;

    return *CurrPos;
}

}
