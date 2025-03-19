#pragma once

#include "link_counters.h"

#include <library/cpp/protobuf/protofile/protofile.h>

#include <library/cpp/deprecated/calc_module/action_points.h>
#include <library/cpp/deprecated/calc_module/stream_points.h>
#include <library/cpp/deprecated/calc_module/misc_points.h>
#include <library/cpp/deprecated/calc_module/simple_module.h>

#include <util/generic/hash.h>
#include <library/cpp/uri/http_url.h>

struct TRTOwnerInfo {
    ui32 Tic;
    ui16 Geo;
    TVector<ui16> Catty;

    // Default values are copies of constant values for yserver.
    TRTOwnerInfo(ui32 tic, ui16 geo, TVector<ui16>& catty)
        : Tic(tic)
        , Geo(geo)
    {
        DoSwap(Catty, catty);
    }
};

template <>
inline void Out<TRTOwnerInfo>(IOutputStream& os, TTypeTraits<TRTOwnerInfo>::TFuncParam ownerInfo) {
    os << "{Tic=" << ownerInfo.Tic << ", Geo=" << ownerInfo.Geo << ", Catty={";
    for(size_t i = 0; i < ownerInfo.Catty.size(); i++)
        os << (i ? ", " : "") << ownerInfo.Catty[i];
    os << "}}";
}

struct TRTHostInfo {
    ui32 OwnerId;

    TRTHostInfo(ui32 ownerId)
        : OwnerId(ownerId)
    {
        Y_ASSERT(ownerId);
    }
};

template <>
inline void Out<TRTHostInfo>(IOutputStream& os, TTypeTraits<TRTHostInfo>::TFuncParam hostInfo) {
    os << "{OwnerId=" << hostInfo.OwnerId << "}";
}

class TRTDatabaseException : public yexception {
};

// zero TIndex = invalid/uninitialized index
template<class TValue, class TIndex = size_t>
class TRTDatabaseTable {
private:
    TVector<TValue> Table;
public:
    inline void Clear() {
        Table.clear();
    }
    inline TValue& GetElement(TIndex index) {
        if (Y_UNLIKELY(!index))
            ythrow TRTDatabaseException() << "not found index " << index << " in table";
        if (Y_UNLIKELY(index > Table.size()))
            ythrow TRTDatabaseException() << "overflow by index " << index << " in table";
        return Table[index - 1];
    }
    inline TIndex AddElement(const TValue& x) {
        Table.push_back(x);
        return Table.size();
    }
    inline TVector<TValue>& GetVector() {
        return Table;
    }
    void PrintDebugInfo(IOutputStream& s) const {
        for(typename TVector<TValue>::const_iterator i = Table.begin(); i != Table.end(); i++)
            s << "    Table: value=" << *i << Endl;
    }
};

// zero TIndex = invalid/uninitialized index
template<class TValue, class TKey, class TIndex = ui32>
class TRTDatabaseIndexedTable : public TRTDatabaseTable<TValue, TIndex> {
private:
    typedef TRTDatabaseTable<TValue, TIndex> TBase;
    typedef THashMap<TKey, TIndex> TIndices;
    TIndices Index;
public:
    inline void Clear() {
        Index.clear();
        TBase::Clear();
    }
    inline TIndex FindIndex(const TKey& key) {
        return Index[key];
    }
    inline TValue& FindElement(const TKey& key) {
        try {
            return TBase::GetElement(FindIndex(key));
        } catch (const TRTDatabaseException& e) {
            ythrow TRTDatabaseException() << "wrong key: " << e.what();
        }
    }
    inline TIndex AddElement(const TKey& key, const TValue& x) {
        TIndex index = FindIndex(key);
        return index ? index : Index[key] = TBase::AddElement(x);
    }
    void PrintDebugInfo(IOutputStream& s) const {
        for(typename TIndices::const_iterator i = Index.begin(); i != Index.end(); i++)
            s << "  Index: key=" << i->first << " value=" << i->second << Endl;
        TBase::PrintDebugInfo(s);
    }
};

typedef TRTDatabaseIndexedTable<TRTOwnerInfo, TString> TOwnersTable;
typedef TRTDatabaseIndexedTable<TRTHostInfo, TString> THostsTable;

class TRTLinkDatabase : public TSimpleModule {
private:
public:
    typedef std::pair<ui32, ui32> TOwnerHandle;

protected:

    TOwnersTable OwnersTable;
    THostsTable HostsTable;

    TLinkCounters IncomeLinksCounters;

    TMasterAnswerPoint<const TString&, TString> GetHostOwnerPoint;

public:
    TRTLinkDatabase()
        : TSimpleModule("TRTLinkDatabase")
        , GetHostOwnerPoint(this, "get_host_owner")
    {
        Bind(this).To<const TString&, ui32&, &TRTLinkDatabase::GetOwnerIdByHost>("ownerid_by_host_output");
        Bind(this).To<const ui32, ui32&, &TRTLinkDatabase::GetTicByOwnerId>("tic_by_ownerid_output");
        Bind(this).To<ui32, ui32&, &TRTLinkDatabase::GetOwnerIdByDocId>("ownerid_by_docid_output");
        Bind(this).To<TOwnerHandle, TOwnerHandle, ui16&, &TRTLinkDatabase::GetNumCatMatches>("get_num_cat_matches");
        Bind(this).To<TOwnerHandle, ui32&, &TRTLinkDatabase::GetTic>("tic_output");
        Bind(this).To<TOwnerHandle, ui16&, &TRTLinkDatabase::GetGeo>("geo_output");
        Bind(this).To<ui32, ui32&, &TRTLinkDatabase::GetNumRefsFromMPPoint>("get_num_refs_from_mp");
        Bind(this).To<const TString&/*url*/, const TAnchorText::TExtraHostFactors&, &TRTLinkDatabase::AddToDatabase>("add_2_db_input");
        Bind(this).To<&TRTLinkDatabase::Start>("start");
        Bind(this).To<&TRTLinkDatabase::Finish>("finish");
        Bind(this).To<const TLinkCounters*>(&IncomeLinksCounters, "generic_link_counts_output");
    }

    static inline TString GetHost(const TString& url) {
        TString schemedUrl = AddSchemePrefix(url);
        THttpURL parsed;
        parsed.Parse(schemedUrl);
        return parsed.PrintS(THttpURL::FlagHostPort);
    }

private:
    void Start() {
        // fix number of internal links (one link is the current page)
        Y_VERIFY(IncomeLinksCounters.Internal > 0, "At least one link must be in database.");
        --IncomeLinksCounters.Internal;
    }

    void Finish() {
        OwnersTable.Clear();
        HostsTable.Clear();
        Init();
    }

    void PrintDebugInfo(const yexception&) {
        Cerr << "Owners table: " << Endl;
        OwnersTable.PrintDebugInfo(Cerr);
        Cerr << "Hosts table: " << Endl;
        HostsTable.PrintDebugInfo(Cerr);
    }

    void Init() {
        IncomeLinksCounters.Reset();
    }

    // AddToDatabase and GetCrawlRank must be called together
    void AddToDatabase(const TString& url, const TAnchorText::TExtraHostFactors& factors) {
        typedef ::google::protobuf::RepeatedField< ::google::protobuf::uint32 >::const_iterator TCattyFieldIterator;
        THttpURL parsed;
        TString host = GetHost(url);
        host.to_lower();
        TString owner = GetHostOwnerPoint.Answer(host);
        TVector<ui16> catty;
        for (TCattyFieldIterator i = factors.GetOwnerCatty().begin(); i != factors.GetOwnerCatty().end(); ++i) {
            catty.push_back(*i);
        }
        TRTOwnerInfo ownerInfo(factors.GetOwnerTic(), factors.HasOwnerGeo() ? factors.GetOwnerGeo() : -1, catty);
        ui32 ownerId = OwnersTable.AddElement(owner, ownerInfo);
        // ownerId == 1 it means host of currnet indexed page are received
        if (ownerId == 1) {
            ++IncomeLinksCounters.Internal;
        } else {
            ++IncomeLinksCounters.External;
        }

        TRTHostInfo hostInfo(ownerId);
        HostsTable.AddElement(host, hostInfo);
    }

    void GetOwnerIdByHost(const TString& hostName, ui32& ownerId) {
        try {
            ownerId = HostsTable.FindElement(hostName).OwnerId;
            if (!ownerId)
                ythrow TRTDatabaseException() << "ownerId == 0";
        } catch (const TRTDatabaseException& e) {
            PrintDebugInfo(e);
            throw;
        }
    }
    void GetTicByOwnerId(ui32 ownerId, ui32& ownerTic) {
        try {
            ownerTic = OwnersTable.GetElement(ownerId).Tic;
        } catch (const TRTDatabaseException& e) {
            PrintDebugInfo(e);
            throw;
        }
    }
    void GetOwnerIdByDocId(ui32 docId, ui32& ownerId) {
        try {
            if (docId != 0)
                ythrow TRTDatabaseException() << "docId isn't equal to zero";
        } catch (const TRTDatabaseException& e) {
            PrintDebugInfo(e);
            throw;
        }
        ownerId = 1;
    }
    void GetNumCatMatches(TOwnerHandle owner1, TOwnerHandle owner2, ui16& numMatches) {
        try {
            ui16 matched = 0;
            ui32 cur1, cur2;
            const TVector<ui16>& catty1 = OwnersTable.GetElement(owner1.second).Catty;
            const TVector<ui16>& catty2 = OwnersTable.GetElement(owner2.second).Catty;
            for (cur1 = 0, cur2 = 0; cur1 != catty1.size() && cur2 != catty2.size();) {
                if (catty1[cur1] == catty2[cur2]) {
                    ++matched;
                    ++cur1;
                    ++cur2;
                } else if (catty1[cur1] < catty2[cur2]) {
                    ++cur1;
                } else {
                    ++cur2;
                }
            }
            numMatches = matched;
        } catch (const TRTDatabaseException& e) {
            PrintDebugInfo(e);
            throw;
        }
    }
    void GetTic(TOwnerHandle owner, ui32& ownerTic) {
        GetTicByOwnerId(owner.second, ownerTic);
    }
    void GetGeo(TOwnerHandle owner, ui16& ownerGeo) {
        try {
            ownerGeo = OwnersTable.GetElement(owner.second).Geo;
        } catch (const TRTDatabaseException& e) {
            PrintDebugInfo(e);
            throw;
        }
    }
    void GetNumRefsFromMPPoint(ui32 /*docId*/, ui32& numRefs) {
        // TODO: add info about link from main page to TextRec.
        numRefs = 0;
    }

};
