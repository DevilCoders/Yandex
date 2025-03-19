#include <library/cpp/charset/codepage.h>
#include <util/charset/wide.h>
#include <library/cpp/string_utils/url/url.h>
#include <kernel/indexer/face/inserter.h>
#include <kernel/indexer/face/docinfo.h>
#include <ysite/yandex/common/prepattr.h>
#include <yweb/robot/dbscheeme/mergecfg.h>
#include "attrconf.h"
#include "geodata.h"

namespace {

    class TGeoAttrFiles : public IGeoAttrStorage {
        TOutDatFileArray<TDocName> Files;
    public:
        TGeoAttrFiles()
            : Files("geoattrfile", dbcfg::pg_url, 1u << 20, 0)
        {
        }
        void Open(ui32 count, const char* fileName) override {
            Files.Open(count, fileName);
        }
        TDocName* Reserve(size_t fileIndex, size_t size) override {
            return (TDocName*)Files[fileIndex].Reserve(size);
        }
    };

}

TGeoData::TGeoData(const TAttrProcessorConfig* cfg, IGeoAttrStorage* storage)
    : GeoAttrStorage(storage ? storage : (IGeoAttrStorage*)new TGeoAttrFiles())
    , GeoDocsFile("geodocsfile", dbcfg::fbufsize, 0)
    , GeoDocsOffFile("geodocsfile", dbcfg::fbufsize, 0)
{
    GeoTrieMap.init(cfg->GeoTrieName.data());
//    GeoTrie.Init((char*)GeoTrieMap.getData(), GeoTrieMap.getSize());
    Trie = (char*)GeoTrieMap.getData();
    ui32 trieCount = *(ui32*)Trie;
    GeoTries.reserve(trieCount);
    ui64* offsets = (ui64*)(Trie + sizeof(ui32));
    ui64 curoff = sizeof(ui32) + trieCount * sizeof(ui64);
    for (ui32 i = 0; i < trieCount; i++) {
        if (offsets[i] == curoff)
            GeoTries.push_back((TGeoTrie*)nullptr);
        else {
            TGeoTrie* tr = new TGeoTrie;
            tr->Init(Trie + curoff, size_t(offsets[i] - curoff));
            GeoTries.push_back(tr);
        }
        curoff = offsets[i];
    }
    char name[PATH_MAX];
    sprintf(name, "%s/walrus/%03d/geoattrs.%%d.dat", cfg->HomeDir.data(), cfg->Cluster);
    GeoAttrStorage->Open(trieCount >> 1, name);
    IsGeoDocs = false;
    IsGeoOff = false;
//    fprintf(stderr, "geodocs %c\n", (~cfg->GeoDocsName)[0]);
    if ((cfg->GeoDocsName.data())[0]) {
        GeoDocsFile.Open(cfg->GeoDocsName.data());
        GeoDoc = GeoDocsFile.Next();
        IsGeoDocs = true;
//        fprintf(stderr, "IsGeoDoc\n");
    } else if ((cfg->GeoDocsOffName.data())[0]) {
        GeoDocsOffFile.Open(cfg->GeoDocsOffName.data());
        GeoDocOff = GeoDocsOffFile.Next();
        IsGeoOff = true;
    }
}

TGeoData::~TGeoData() {
    for (std::vector<TGeoTrie*>::iterator it = GeoTries.begin(); it != GeoTries.end(); it++) {
        if (*it)
            delete *it;
    }
}

void PutExtraAttrs(IDocumentDataInserter& inserter, char* pst)
{
    while (*pst) {
        char* est = strchr(pst, 7);
        if (est)
            *est = 0;
        const TUtf16String val(UTF8ToWide(pst));
        inserter.StoreLiteralAttr("nv", val.data(), val.size(), 0);
        if (est) {
            *est = 7;
            pst = est + 2;
        } else
            return;
    }
}

void TGeoData::StoreGeoAttrs(IDocumentDataInserter& inserter, const char *url, ui32 docId, ui32 group) {
    TGeo val;
    size_t prefixLen;
    if ((group < GeoTries.size()) && GeoTries[group] &&
        (group & 1 ? GeoTries[group]->Find(url, strlen(url), &val)
                   : GeoTries[group]->FindLongestPrefix(url, strlen(url), &prefixLen, &val))) {
        StoreGeoAttrs(inserter, docId, &val, group >> 1);
    }
}

void TGeoData::StoreGeoAttrs(IDocumentDataInserter& inserter, ui64 offset, ui32 docId, ui32 minibase) {
    TGeo val;
    Packer.UnpackLeaf(Trie + offset, val);
    StoreGeoAttrs(inserter, docId, &val, minibase);
}

void TGeoData::StoreGeoAttrs(IDocumentDataInserter& inserter, ui32 docId, TGeo* val, ui32 minibase) {
        ui32 locationid = val->GetLine();
        if (val->HasSt())
            PutExtraAttrs(inserter, const_cast<char*>(val->GetSt().data()));
        if (val->HasAux())
        {
            char* aux = const_cast<char*>(val->GetAux().data());
            PutExtraAttrs(inserter, aux);
            size_t len = strlen(aux);
            if (len < URL_MAX) {
                TDocName* dn = GeoAttrStorage->Reserve(minibase, sizeof(ui32) + len + 1);
                dn->DocId = docId;
                strcpy(dn->Name, aux);
            }
        }
        if (val->HasGeo())
        {
            char* xy = const_cast<char*>(val->GetGeo().data());
            while (1)
            {
                char* nx = strchr(xy, 6);
                size_t len;
                if (nx)
                {
                    *nx = 0;
                    len = nx - xy;
                }
                else
                    len = strlen(xy);
                if (len < URL_MAX) {
                    TDocName* dn = GeoAttrStorage->Reserve(minibase, sizeof(ui32) + len + 1);
                    dn->DocId = docId;
                    strcpy(dn->Name, xy);
                }
                if (nx == nullptr)
                    break;
                *nx = 6;
                xy = nx + 2;
            }
            char cloc[12];
            sprintf(cloc, "%i", locationid);
            TDocName* dn = GeoAttrStorage->Reserve(minibase, sizeof(ui32) + strlen(cloc) + 12);
            dn->DocId = docId;
            sprintf(dn->Name, "locationid=%s", cloc);
        }
}

void TGeoData::StoreGeoAttrs(IDocumentDataInserter& inserter, const TDocInfoEx& docInfo) {
    const ui32 docId = docInfo.DocId;

    if (IsGeoOff) {
        StoreGeoAttrsByOff(inserter, docId);
        return;
    }

    const char* url = docInfo.DocHeader->Url;
    url += GetHttpPrefixSize(url);
    const size_t bufSize = 2 * FULLURL_MAX;
    char buf[bufSize];
    const ECharset enc = (ECharset)docInfo.DocHeader->Encoding;
    if (SingleByteCodepage(enc)) {
        const size_t len = FixNationalUrl(url, buf, bufSize - 1, enc);
        buf[len] = 0;
        url = buf;
    }

    int cmp = 1;
//    ui32 group = 0;
    if (IsGeoDocs)
    {
        while ((cmp = GeoDoc ? ::compare((ui32)GeoDoc->DocId, docId) : 1) < 0)
            GeoDoc = GeoDocsFile.Next();
//        if (cmp == 0)
//            group = GeoDoc->GroupId;
    }
    StoreGeoAttrs(inserter, url, docId, 0);
    StoreGeoAttrs(inserter, url, docId, 1);
    while (cmp == 0)
    {
        StoreGeoAttrs(inserter, url, docId, GeoDoc->GroupId << 1);
        StoreGeoAttrs(inserter, url, docId, (GeoDoc->GroupId << 1) + 1);
        if (!IsGeoDocs)
            break;
        GeoDoc = GeoDocsFile.Next();
        cmp = GeoDoc ? ::compare((ui32)GeoDoc->DocId, docId) : 1;
//        if (cmp == 0)
//            group = GeoDoc->GroupId;
    }
}

void TGeoData::StoreGeoAttrsByOff(IDocumentDataInserter& inserter, ui32 docId) {
    int cmp = 1;
//    ui32 group = 0;
    while ((cmp = GeoDocOff ? ::compare((ui32)GeoDocOff->DocId, docId) : 1) < 0)
        GeoDocOff = GeoDocsOffFile.Next();
    while (cmp == 0) {
        StoreGeoAttrs(inserter, GeoDocOff->Crc, docId, GeoDocOff->HostId);
        GeoDocOff = GeoDocsOffFile.Next();
        cmp = GeoDocOff ? ::compare((ui32)GeoDocOff->DocId, docId) : 1;
    }
}

