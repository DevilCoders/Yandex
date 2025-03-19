#pragma once

#include <util/generic/vector.h>
#include <library/cpp/containers/comptrie/comptrie.h>
#include <library/cpp/containers/comptrie/protopacker.h>
#include <library/cpp/microbdb/safeopen.h>
#include <yweb/protos/geo.pb.h>
#include <yweb/robot/dbscheeme/baserecords.h>
#include <yweb/robot/dbscheeme/docnamerecords.h>

struct TDocInfoEx;
struct TAttrProcessorConfig;
class IDocumentDataInserter;

class IGeoAttrStorage : public TSimpleRefCount<IGeoAttrStorage> {
public:
    virtual ~IGeoAttrStorage() {}
    virtual void Open(ui32 count, const char* fileName) = 0;
    virtual TDocName* Reserve(size_t fileIndex, size_t size) = 0;
};

class TGeoData {
public:
    TGeoData(const TAttrProcessorConfig* cfg, IGeoAttrStorage* storage);
    ~TGeoData();

    void StoreGeoAttrs(IDocumentDataInserter& inserter, const TDocInfoEx& docInfo);

private:
    void StoreGeoAttrs(IDocumentDataInserter& inserter, ui32 docId, TGeo* val, ui32 minibase);
    void StoreGeoAttrs(IDocumentDataInserter& inserter, ui64 offset, ui32 docId, ui32 minibase);
    void StoreGeoAttrs(IDocumentDataInserter& inserter, const char *url, ui32 docId, ui32 group);
    void StoreGeoAttrsByOff(IDocumentDataInserter& inserter, ui32 docId);

private:
    typedef TCompactTrie<char, TGeo, TProtoPacker<TGeo> > TGeoTrie;

    TMappedFile GeoTrieMap;
    TVector<TGeoTrie*> GeoTries;
    char* Trie;
    TProtoPacker<TGeo> Packer;
//    TCompactTrie<char, TString> GeoTrie;
    const TIntrusivePtr<IGeoAttrStorage> GeoAttrStorage;
    TInDatFile<TDocGroupRec> GeoDocsFile;
    const TDocGroupRec* GeoDoc;
    TInDatFile<TSigDocRec> GeoDocsOffFile;
    const TSigDocRec* GeoDocOff;
    bool IsGeoDocs;
    bool IsGeoOff;
};

