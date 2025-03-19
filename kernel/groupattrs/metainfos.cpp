#include "metainfos.h"

#include <util/folder/dirut.h>
#include <util/system/file.h>
#include <util/system/fs.h>

namespace NGroupingAttrs {

void TMetainfos::Init(const char* indexname, bool lockMemory) {
    // GetDirName used for win - may be path like as a\b\v/root/index1. Mixed slashes;
    TString dir = GetDirName(indexname);
    if (indexname == dir)
        dir = ".";

    Metainfos.resize(Config.AttrCount());
    for (ui32 attrnum = 0; attrnum < Config.AttrCount(); ++attrnum) {
        ParseFile(dir, attrnum, lockMemory);
    }
}

void TMetainfos::Clear() {
    Metainfos.clear();
}

void TMetainfos::ParseFile(const TString& indexdir, ui32 attrnum, bool lockMemory) {
    const TString attrname = Config.AttrName(attrnum);
    TString filename = TString::Join(indexdir, "/", attrname);
    bool useWad = NFs::Exists(filename + ".c2n.wad");

    Metainfos[attrnum].Reset(new TMetainfo(DynamicC2N, useWad, lockMemory));
    TMetainfo& mi = *(Metainfos[attrnum].Get());
    if (useWad) {
        mi.LoadC2NOffroadWad(filename, attrname);
    } else {
        mi.LoadC2N(filename, attrname);
    }

    mi.LoadMap(TMetainfo::C2P, false, (filename + ".c2p").data());
    mi.LoadMap(TMetainfo::C2Co, false, (filename + ".c2s").data());
    mi.LoadMap(TMetainfo::C2L, false, (filename + ".c2l").data());
}

const char* TMetainfos::CategName(const TCateg& aclass, const char* attrname) const {
    if (!attrname) {
        return NONAME;
    }

    const TMetainfo* mi = Metainfo(attrname);
    if (!mi) {
        return NONAME;
    }

    return mi->Categ2Name(aclass);
}

TCateg TMetainfos::CategParent(const TCateg& aclass, const char* attrname) const {
    if (!attrname) {
        return END_CATEG;
    }

    const TMetainfo* mi = Metainfo(attrname);
    if (!mi) {
        return END_CATEG;
    }

    return mi->Categ2Parent(aclass);
}

bool TMetainfos::IsLink(const char* attrname, const TCateg& parent, const TCateg& link) const {
    if (!attrname) {
        return false;
    }

    const TMetainfo* mi = Metainfo(attrname);
    if (!mi) {
        return false;
    }

    return mi->IsLink(parent, link);
}

}
