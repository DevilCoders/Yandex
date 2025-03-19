#pragma once

#include <kernel/indexer/direct_text/dt.h>
#include <library/cpp/token/nlptypes.h>
#include <kernel/indexer_iface/yandind.h>

#include <library/cpp/charset/ci_string.h>
#include <util/charset/wide.h>
#include <util/generic/hash.h>
#include <util/generic/algorithm.h>
#include <util/memory/segmented_string_pool.h>
#include <library/cpp/string_utils/base64/base64.h>

namespace NIndexerCore {
namespace NIndexerCorePrivate {

class TDirectZones {

    struct TDZAttrKey {
        const TCiString Name;
        const ui8 Type;

        inline TDZAttrKey()
            : Type(0)
        { }

        inline TDZAttrKey(const TCiString& name, ui8 type)
            : Name(name)
            , Type(type)
        { }

        inline bool operator == (const TDZAttrKey& other) const {
            return Name == other.Name && Type == other.Type;
        }

        struct THash {
            inline size_t operator () (const TDZAttrKey& key) const {
                return ComputeHash(key.Name) + key.Type;
            }
        };
    };

public:
    typedef TVector<TDirectTextZone> TZones;
    typedef TVector<TDirectTextZoneAttr> TAttrs;

     TDirectZones();
    ~TDirectZones();

    size_t Capacity() const;

    void Clear();

    void StoreZone(ui8 ztype, const TStringBuf& key, TPosting pos);

    void StoreZone(ui8 ztype, const TStringBuf& key, TPosting beg, TPosting end);

    void StoreArchiveZoneAttr(ui8 type, const TStringBuf& name, const TWtringBuf& value, TPosting pos, bool noFollow, bool sponsored, bool ugc);

    void Prepare();

    inline const TZones& GetZones() const {
        return Zones;
    }

    inline const TAttrs& GetAttrs() const {
        return Attrs;
    }


private:
    TVector<TZoneSpan>& InsertOrGetZone(const TCiString& key, const ui8 ztype);

private:
    typedef THashMap<TCiString, std::pair<ui8, TVector<TZoneSpan> > > TZoneMap;
    typedef THashMap<TDZAttrKey, TVector<TDirectAttrEntry>, TDZAttrKey::THash, TEqualTo<TDZAttrKey> > TAttrMap;

    TZoneMap ZoneMap;
    TAttrMap AttrMap;
    TZones Zones;
    TAttrs Attrs;
};


struct TDirectData {
    TDirectTextData2 DirectText;
    // Just for store merged zones and attrs.
    TVector<TDirectTextZone> Zones;
    TVector<TDirectTextZoneAttr> ZoneAttrs;
};


class TDirectText2 {

    class TDirectAttrs {
    public:
        typedef TVector<TDirectTextSentAttr> TAttrs;

        TDirectAttrs() {
            Attrs.reserve(256);
        }

        void Clear() {
            Attrs.clear();
        }

        void StoreAttr(const char * key, const TString& val, TPosting pos) {
            Attrs.emplace_back();
            Attrs.back().Sent = GetBreak(pos);
            Attrs.back().Attr = key;
            Attrs.back().Value = val;
        }

        void Prepare() {
            Sort(Attrs.begin(), Attrs.end());
        }

        const TAttrs& GetAttrs() const {
            return Attrs;
        }

    private:
        TAttrs Attrs;
    };

public:
    const bool KiwiTrigger; // don't copy spaces and attribute values when triggers work

    explicit TDirectText2(size_t wordCount, bool kiwiTrigger = false);

    void Clear();

    TDirectTextEntries GetEntries();

    size_t MemUsage() const;

    void InsertSpace(const wchar16* space, ui32 length, TBreakType type);

    void InsertForm(const TWtringBuf& token, TLemmatizedToken* forms, ui32 count, TPosting pos, ui32 origoffset);

    void StoreZone(ui8 type, const TStringBuf& key, TPosting pos);

    void StoreZoneAttr(ui8 type, const TStringBuf& name, const TWtringBuf& value, TPosting pos, bool noFollow, bool sponsored, bool ugc);

    void OnCommitDoc();
    //! @attention OnCommitDoc() must be called before call to this function.
    void FillDirectData(TDirectData* data, const TDirectTextData2* extraDirectText = nullptr) const;

private:
    void AdjustLastEntry(TPosting pos);

private:
    segmented_pool<wchar16> Pool;
    segmented_pool<TDirectTextSpace> SpacePool;
    TVector<TDirectTextSpace> TempSpaces;
    TVector<TDirectTextEntry2> Entries;
    TDirectZones DirectZones;
    TDirectAttrs DirectAttrs;
};

} // NIndexerCorePrivate
} // NIndexerCore
