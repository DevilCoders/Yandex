#include <util/generic/map.h>
#include <util/generic/strbuf.h>
#include <util/string/cast.h>
#include <util/string/reverse.h>
#include <util/string/split.h>
#include <util/stream/file.h>
#include <util/folder/dirut.h>

#include "clon.h"

TClonAttrCanonizer::TClonAttrCanonizer()
    : Pool(new TMemoryPool(1u << 20))
{}

TClonAttrCanonizer::TClonAttrCanonizer(const TString& fname)
    : Pool(new TMemoryPool(1u << 20))
{
    LoadClons(fname);
}

TClonAttrCanonizer::TClonAttrCanonizer(const TString& clonh2gTrieIndex, const TString& clonh2gTrieData) {
    ClonTrieIndex.Reset(new TClonTrieIndex(TBlob::FromFile(clonh2gTrieIndex)));
    ClonTrieData = TBlob::FromFile(clonh2gTrieData);
}

void TClonAttrCanonizer::GetHostClonGroups(TStringBuf host, TVector<ui32>& groups) const {
    bool useTrie = !!ClonTrieIndex;
    groups.clear();
    typedef TMap<ui32, bool> TGroupSet;
    TGroupSet add;

    // Check each domain of host for clon groups
    for (TStringBuf d = host; d.size();) {
        const TGroup* begin = nullptr;
        const TGroup* end = nullptr;
        if (useTrie) {
            TString str(ToString(d));
            ReverseInPlace(str);
            TDataIndices dataIndices;
            if (ClonTrieIndex->Get(str.data(), &dataIndices)) {
                begin = ((const TGroup*)ClonTrieData.Data()) + dataIndices.FirstIndex;
                end = ((const TGroup*)ClonTrieData.Data()) + dataIndices.LastIndex;
            }
        } else {
            TClonMap::const_iterator hostGroups = ClonMap.find(d);
            if (hostGroups != ClonMap.end()) {
                begin = hostGroups->second.data();
                end = begin + hostGroups->second.size();
            }
        }

        // For each clon group check domain status in group
        for (const TGroup* i = begin; i != end; ++i) {
            const TGroup& g = *i;
            // Host is added to group if:
            // 1) Owner is in group and host is not explicitly removed from it (Type == 1)
            // 2) Host is explicitly added to group (Type == 3)
            if (add.find(g.Group) == add.end()) {
                if ((g.Type == 3 && host == d) || g.Type == 1) {
                    // Exact matches are added explicitly, non-exact - only with all owner's hosts
                    add[g.Group] = true;
                } else if (g.Type == 2 && host == d) {
                    // Only exact matches are removed explicitly
                    add[g.Group] = false;
                }
            }
        }
        d = d.SplitOff('.');
    }
    for (TGroupSet::const_iterator i = add.begin(); i != add.end(); ++i) {
        if (i->second)
            groups.push_back(i->first);
    }
}

void TClonAttrCanonizer::AddClon(TStringBuf host, ui32 group, ui32 type) {
    TGroup g = {group, type};
    while (host.size() && host[host.size() - 1] == '/')
        host.Chop(1);
    TClonMap::iterator i = ClonMap.find(host);
    if (i == ClonMap.end())
        i = (ClonMap.insert(std::make_pair(TStringBuf(Pool->Append(host.data(), host.size()), host.size()), TGroupVector()))).first;
    i->second.push_back(g);
}

void TClonAttrCanonizer::LoadClons(const TString& fname) {
    if (fname.size()) {
        TFileInput input(fname);
        TString s;
        TVector<TStringBuf> fields;
        while (input.ReadLine(s)) {
            fields.clear();
            StringSplitter(s).Split('\t').AddTo(&fields);
            if (fields.size() < 3)
                continue;
            AddClon(fields[0], FromString<ui32>(fields[1]), FromString<ui32>(fields[2]));
        }
    }
}
