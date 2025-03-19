#pragma once

#include <kernel/indexer/face/inserter.h>

#include <util/generic/buffer.h>
#include <util/generic/hash.h>
#include <util/memory/pool.h>

namespace NIndexerCore {

class TDataContainer : public IDocumentDataInserter {
public:
    static const char* LEMMA_KEY;
    explicit TDataContainer(size_t initialPoolSize = 1024);

    void StoreLiteralAttr(const char* attrName, const char* attrText, TPosting pos) override;
    void StoreLiteralAttr(const char* attrName, const wchar16* attrText, size_t len, TPosting pos) override;
    void StoreDateTimeAttr(const char* attrName, time_t datetime) override;
    void StoreIntegerAttr(const char* attrName, const char* attrText, TPosting pos) override;
    void StoreKey(const char* key, TPosting pos) override;
    void StoreZone(const char* zoneName, TPosting begin, TPosting end, bool archiveOnly) override;
    void StoreArchiveZoneAttr(const char* name, const wchar16* value, size_t length, TPosting pos) override;
    void StoreLemma(const wchar16* lemma, size_t lemmaLen, const wchar16* form, size_t formLen, ui8 flags, TPosting pos, ELanguage lang) override;
    void StoreTextArchiveDocAttr(const TString& name, const TString& value) override;
    void StoreFullArchiveDocAttr(const TString& name, const TString& value) override;
    void StoreErfDocAttr(const TString&, const TString&) override;
    void StoreGrpDocAttr(const TString&, const TString&, bool) override;

    // an additional non-virtual member function
    void StoreUrlAttr(const char* name, const char* text, TPosting pos);

    //! Save data to buffer.
    TBuffer SerializeToBuffer() const;

private:
    struct TDataItem;

    // \return pointer to allocated data buffer.
    void* AllocateRecord(TDataItem* tag, ui8 type, size_t len);

    TDataItem* InsertName(const char* name);

    void InsertTextValueItem(TDataItem* tag, ui8 type, const wchar16* text, size_t len, ui32 pos);

    void InsertUI32ValueItem(TDataItem* tag, ui8 type, ui32 pos);

    void SaveToImpl(TBuffer* buf) const;

private:
    typedef THashMap<const char*, TDataItem*> TNameSet;

    TMemoryPool Pool_;
    TNameSet    Names_;
    size_t      Length_;
};


void InsertDataTo(const TStringBuf& data, IIndexDataInserter* indexDataInserter, IArchiveDataInserter* archiveDataInserter);

bool IsDataContainer(const TStringBuf& data);

//! Merge list of data-containes into a single result buffer.
void MergeDataContainer(const TVector<TStringBuf>& data, TBuffer* result);

} // namespace NIndexerCore
