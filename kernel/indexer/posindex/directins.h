#pragma once

#include <kernel/indexer/face/inserter.h>
#include <kernel/indexer/faceproc/dains.h>

#include <util/generic/noncopyable.h>

namespace NIndexerCore {

class TInvCreator;

class TInvCreatorInserter : public TInserterToDocAttrs, TNonCopyable {
private:
    TInvCreator& Creator;
public:
    TInvCreatorInserter(TInvCreator& data, TFullDocAttrs* docAttrs);

    void StoreLiteralAttr(const char* attrName, const char* attrText, TPosting pos) override;
    void StoreLiteralAttr(const char* attrName, const wchar16* attrText, size_t len, TPosting pos) override;
    void StoreDateTimeAttr(const char* attrName, time_t datetime) override;
    void StoreIntegerAttr(const char* attrName, const char* attrText, TPosting pos) override;
    void StoreKey(const char* key, TPosting pos) override;
    void StoreZone(const char* zoneName, TPosting begin, TPosting end, bool archiveOnly = false) override;
    void StoreArchiveZoneAttr(const char* name, const wchar16* value, size_t length, TPosting pos) override;
    void StoreLemma(const wchar16* lemma, size_t lemmaLen, const wchar16* form, size_t formLen, ui8 flags, TPosting pos, ELanguage lang) override;
};

}
