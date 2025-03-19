#pragma once

#include <kernel/indexer/face/directtext.h>
#include <kernel/indexer/face/inserter.h>

#include <util/memory/segmented_string_pool.h>
#include <util/generic/buffer.h>

namespace NIndexerCore {

class TDirectZoneInserter
    : public  IDocumentDataInserter
    , private TNonCopyable
{
public:
    TDirectZoneInserter();

    void StoreLiteralAttr(const char*, const char*, TPosting) override {
        throw "not implemented";
    }
    void StoreLiteralAttr(const char*, const wchar16*, size_t, TPosting) override {
        throw "not implemented";
    }
    void StoreDateTimeAttr(const char*, time_t) override {
        throw "not implemented";
    }
    void StoreIntegerAttr(const char*, const char*, TPosting) override {
        throw "not implemented";
    }
    void StoreKey(const char*, TPosting) override {
        throw "not implemented";
    }

    void StoreZone(const char* zoneName, TPosting begin, TPosting end, bool archiveOnly = false) override;
    void StoreArchiveZoneAttr(const char* name, const wchar16* value, size_t length, TPosting pos) override;

    void StoreLemma(const wchar16*, size_t, const wchar16*, size_t, ui8, TPosting, ELanguage) override {
        throw "not implemented";
    }
    void StoreTextArchiveDocAttr(const TString&, const TString&) override {
        throw "not implemented";
    }
    void StoreFullArchiveDocAttr(const TString&, const TString&) override {
        throw "not implemented";
    }
    void StoreErfDocAttr(const TString&, const TString&) override {
        throw "not implemented";
    }
    void StoreGrpDocAttr(const TString&, const TString&, bool) override {
        throw "not implemented";
    }

    void PrepareZones();

    TBuffer SerializeZones() const;

private:
    NIndexerCorePrivate::TDirectZones Zones_;
    segmented_pool<wchar16> Pool_;
};

} // namespace NIndexerCore
