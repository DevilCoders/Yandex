#include "remaptable.h"

#include <kernel/groupattrs/metainfo.h>

#include <library/cpp/deprecated/mapped_file/mapped_file.h>

#include <util/generic/map.h>
#include <util/generic/set.h>
#include <util/generic/utility.h>
#include <util/memory/pool.h>
#include <util/string/cast.h>
#include <util/string/printf.h>
#include <util/system/filemap.h>

using namespace NM2N;

const ui32 TBaseRemapInfo::EmptyDoc(Max<ui32>());
const ui16 TBaseRemapInfo::EmptyCl(Max<ui16>());
const ui32 TMultipleRemapTableImpl::EmptyNext(Max<ui32>());

namespace NM2N {

void TBaseRemapData::ResizeHolders(const TVector<ui32>& srcDocsCount) {
    ClusterIds.resize(srcDocsCount.size());
    DocIds.resize(srcDocsCount.size());

    for (ui16 cl = 0; cl < srcDocsCount.size(); ++cl) {
        ClusterIds[cl].resize(srcDocsCount[cl], TBaseRemapInfo::EmptyCl);
        DocIds[cl].resize(srcDocsCount[cl], TBaseRemapInfo::EmptyDoc);
    }
}

void TRemapTableImpl::AddRemapNext(ui32 srcDocId, ui16 srcClId, ui32 /*dstDocId*/, ui16 /*dstClId*/) {
    ythrow yexception() << "Source with clId=" << srcClId << ", docId=" << srcDocId << " has multiple destinations" << Endl;
}

bool TRemapTableImpl::GetDst(ui32 /*srcDocId*/, ui16 /*srcClId*/, ui32* /*dstDocId*/, ui16* /*dstClId*/) const {
    return true;
}

bool TRemapTableImpl::GetMultipleDst(ui32 /*srcDocId*/, ui16 /*srcClId*/, TVector<TDocCl>* /*result*/) const {
    return true;
}

void TMultipleRemapTableImpl::AddRemapNext(ui32 srcDocId, ui16 srcClId, ui32 dstDocId, ui16 dstClId) {
    ClusterIds[BufferClId].push_back(dstClId);
    DocIds[BufferClId].push_back(dstDocId);
    Nexts[BufferClId].push_back(EmptyNext);

    ui16 nextClId = srcClId;
    ui32 nextDocId = srcDocId;

    while (!IsNextEmpty(nextClId, nextDocId)) {
        nextDocId = Nexts[nextClId][nextDocId];
        nextClId = BufferClId;
    }

    Nexts[nextClId][nextDocId] = ClusterIds[BufferClId].size() - 1;
}

void TMultipleRemapTableImpl::ResizeHolders(const TVector<ui32>& srcDocsCount) {
    TRemapTableImpl::ResizeHolders(srcDocsCount);

    Nexts.resize(srcDocsCount.size());

    for (ui16 cl = 0; cl < srcDocsCount.size(); ++cl) {
        Nexts[cl].resize(srcDocsCount[cl], EmptyNext);
    }

    SetBufferClId(srcDocsCount.size());
}

bool TMultipleRemapTableImpl::GetDst(ui32 srcDocId, ui16 srcClId, ui32* /*dstDocId*/, ui16* /*dstClId*/) const {
    if (!IsNextEmpty(srcClId, srcDocId)) {
        ythrow yexception() << "Remap with srcClId=" << srcClId << ", srcDocId=" << srcDocId << " has multiple destinations";
    }

    return true;
}

bool TMultipleRemapTableImpl::GetMultipleDst(ui32 srcDocId, ui16 srcClId, TVector<TDocCl>* result) const {
    ui16 nextClId = srcClId;
    ui32 nextDocId = srcDocId;
    while (!IsNextEmpty(nextClId, nextDocId)) {
        nextDocId = Nexts[nextClId][nextDocId];
        nextClId = BufferClId;

        result->push_back(TDocCl(DocIds[BufferClId][nextDocId], ClusterIds[BufferClId][nextDocId]));
    }

    return true;
}

}

static TString GetDomainsFilename(const char* attr, const TString &infile) {
    TString c2nPath = Sprintf("%s.c2n", attr).data();

    size_t pos = infile.find_last_of('/');
    if (pos != TString::npos) {
        c2nPath = infile.substr(0, pos) + "/" + c2nPath;
    }

    return c2nPath;
}

typedef std::pair<TCateg, char*> TCategName;

class TCategNames {
public:
    TCategNames()
        : Pool(0xFFFF)
    {}

    inline TCategName operator[](size_t pos) const {
        return Names[pos];
    }

    inline void Add(TCateg cat, const char* catName, size_t catNameLen) {
        char* alloc = static_cast<char*>(Pool.Allocate(catNameLen + 1));
        memcpy(alloc, catName, catNameLen);
        alloc[catNameLen] = 0;
        Names.push_back(TCategName(cat, alloc));
    }

    inline size_t Size() const {
        return Names.size();
    }

private:
    TMemoryPool Pool;
    TVector<TCategName> Names;
};

class TDstDomainMap {
public:
    TDstDomainMap(TMemoryPool* pool)
        : Pool(pool)
    {}

    inline void Add(TCateg cat, const char* catName, size_t catNameLen) {
        char* alloc = static_cast<char*>(Pool->Allocate(catNameLen + 1));
        memcpy(alloc, catName, catNameLen);
        alloc[catNameLen] = 0;
        Name2Categ.insert(std::pair<char*, TCateg>(alloc, cat));
    }

    inline TCateg operator[](const char* name) const {
        TName2Categ::const_iterator it = Name2Categ.find(name);

        if (it != Name2Categ.end()) {
            return it->second;
        } else {
            return 0;
        }
    }

private:
    typedef THashMap<const char*, TCateg> TName2Categ;

    TMemoryPool* Pool;
    TName2Categ Name2Categ;
};

template <typename TContainer>
inline static void PushDomainName(TContainer* c, const char* s1, size_t l1, const char* s2, size_t l2) {
    c->Add(FromString<TCateg>(s1, l1), s2, l2);
}

template <typename TContainer>
void FillDomain(const char* fileName, TContainer& categNames) {
    TMappedFile f(fileName);
    const char* begin = reinterpret_cast<const char*>(f.getData());
    NGroupingAttrs::ScanContainer(begin, begin + f.getSize(), &categNames, PushDomainName<TContainer>);
}

void MakeOwnersRemapTable(const autoarray<const char *>& infiles, const autoarray<const char*>& outfiles, const char *attr, TMultipleRemapTable *res, bool avoidDuplicates) {

    typedef TSet<TCateg> TdocSet;
    typedef TMap<ui16, TdocSet> TdestinationMap;
    TdestinationMap destinationMap;

    const size_t srcClusterCount = infiles.size();
    const size_t dstClusterCount = outfiles.size();

    res->ClusterIds.resize(srcClusterCount);
    res->DocIds.resize(srcClusterCount);
    res->Nexts.resize(srcClusterCount);

    res->SetBufferClId(srcClusterCount);

    TVector<TDstDomainMap> dstDomains;
    dstDomains.reserve(dstClusterCount);

    TMemoryPool pool(0xFFFF);

    for (ui16 dstCluster = 0; dstCluster < dstClusterCount; ++dstCluster) {
        dstDomains.push_back(TDstDomainMap(&pool));
        FillDomain(GetDomainsFilename(attr, outfiles[dstCluster]).data(), dstDomains.back());
    }

    for (ui16 srcCluster = 0; srcCluster < srcClusterCount; ++srcCluster) {
        TCategNames srcDomainNames;
        FillDomain(GetDomainsFilename(attr, infiles[srcCluster]).data(), srcDomainNames);

        size_t srcDocCount = 0;
        for (size_t i = 0; i < srcDomainNames.Size(); ++i) {
            Y_ASSERT(srcDomainNames[i].first >= 0);
            const size_t srcDoc = static_cast<size_t>(srcDomainNames[i].first);

            srcDocCount = Max<int>(srcDoc + 1, srcDocCount);
        }

        res->ClusterIds[srcCluster].resize(srcDocCount, TBaseRemapInfo::EmptyCl);
        res->DocIds[srcCluster].resize(srcDocCount, TBaseRemapInfo::EmptyDoc);
        res->Nexts[srcCluster].resize(srcDocCount, TMultipleRemapTableImpl::EmptyNext);

        for (size_t i = 0; i < srcDomainNames.Size(); ++i) {
            const TCateg srcDoc = srcDomainNames[i].first;
            const char *name = srcDomainNames[i].second;

            for (ui16 dstCluster = 0; dstCluster < dstClusterCount; ++dstCluster) {
                TCateg dstDoc = dstDomains[dstCluster][name];

                if (dstDoc != 0) {
                    if (avoidDuplicates) {
                        // When merging THostErfInfo, it is unclear what to do if any values differ, so let's avoid duplicates at this stage :
                        // will consider the order of infiles as "sorted by decreasing priority" and not add same host to destination cluster
                        TdestinationMap::const_iterator dest_itr = destinationMap.find(dstCluster);
                        if (dest_itr != destinationMap.end()) {
                            TdocSet::const_iterator set_itr = dest_itr->second.find(dstDoc);
                            if (set_itr != dest_itr->second.end()) {
                                // This item was already pushed from a previous cluster, do not add duplicate
                                continue;
                            }
                        }

                    }
                    res->AddRemap(srcDoc, srcCluster, dstDoc, dstCluster);
                    res->UpdateMaxDstClusterAndDocId(dstCluster, dstDoc);
                    if (avoidDuplicates) {
                        destinationMap[dstCluster].insert(dstDoc);
                    }
                }
            }
        }
    }
}


