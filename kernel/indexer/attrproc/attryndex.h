#pragma once

#include "url_transliterator.h"

#include <kernel/indexer/face/inserter.h>

#include <library/cpp/deprecated/fgood/fgood.h>
#include <util/generic/hash.h>
#include <util/memory/blob.h>

class TAttrStacker {
public:
    TAttrStacker();

    void StoreDocAttrInt(const char* attrname, const char* attrValue);
    void StoreDocAttrLit(const char* attrname, const char* attrValue);
    void StoreDocAttrLit(const char* attrname, const wchar16* attrValue);
    void StoreDocAttrUrl(const char* attrname, const char* attrValue);
    void StoreDocAttrUrl(const char* attrname, const wchar16* attrValue);
    void MoveAttrPos(const char* name);
    void StoreDateTime(const char* attrname, time_t datetime);
    void StoreUrl(const TString& url, ELanguage lang, const TBlob* tokSplitData = nullptr);
    void StoreUrl(const TString& url, ELanguage lang, const TVector<TString>& urlTransliterationData, const TBlob* tokSplitData = nullptr);
    void StoreZone(const char* name, TPosting begin, TPosting end);

    void Clear();
    void SetInserter(IDocumentDataInserter* inserter) {
        Inserter = inserter;
    }

private:
    IDocumentDataInserter* Inserter;

    typedef THashMap<const char*, ui8, THash<const char*>, TEqualTo<const char*> > TAttrPosHash;
    TAttrPosHash AttrPosHash;
    TUrlTransliterator::TTransliteratorCache TransliteratorCache;

private:
    TPosting GetNextAttrPos(const char *name);
    bool StoreUrlTransliteratorItem(const TUrlTransliteratorItem& item);
};
