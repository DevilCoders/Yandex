#pragma once

#include <util/generic/ptr.h>
#include <util/generic/hash_set.h>
#include <util/generic/vector.h>

#include <kernel/indexer/face/docinfo.h>
#include <kernel/catfilter/catfilter_wrapper.h>
#include <library/cpp/microbdb/safeopen.h>
#include <library/cpp/microbdb/compressed.h>
#include <yweb/robot/dbscheeme/baserecords.h>

class TCatClosure;
class IDocumentDataInserter;
class TMultilanguageDocData;
struct TAttrProcessorConfig;
struct TDocName;

class TCatData {
public:
    explicit TCatData(const TAttrProcessorConfig* cfg, const ICatFilter* filter = nullptr, TMultilanguageDocData* multilangDocData = nullptr);
    ~TCatData();

private:
    void FillCatSet(const char* url, const ui32 docId, THashSet<ui32>* res);
    void FillCatVector(const THashSet<ui32>& catSet, TVector<ui32>* res) const;
    void ProcessSearchCatAttr(IDocumentDataInserter* inserter, const THashSet<ui32>& catSet) const;

public:
    bool IsLoaded() const {
        return (HasCatData || HasDatCatData);
    }
    void ProcessCatAttrs(IDocumentDataInserter* inserter, const TDocInfoEx& docInfo, TVector<ui32>& cats);

private:
    bool HasCatData;
    bool HasDatCatData;
    bool HasLocalCatData;
    bool HasMirrorsData;

    THolder<const ICatFilter> Filter;
    THolder<const ICatFilter> LocalFilter;
    THolder<TCatClosure> CatClosure;
    THolder<TMirrorsMappedTrie> Mirrors;
    TInDatFile<TDocGroupRec, TCompressedBufferedInputPageFile> Attrs;
    const TDocGroupRec* CurAttr;
    TMultilanguageDocData* MultilanguageDocData;
};
