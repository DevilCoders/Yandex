#pragma once

#include <kernel/indexer/face/inserter.h>
#include "docattrs.h"

namespace NIndexerCore {

class TInserterToDocAttrs : public IDocumentDataInserter {
protected:
    TFullDocAttrs* DocAttrs;
public:
    TInserterToDocAttrs(TFullDocAttrs* docAttrs)
        : DocAttrs(docAttrs)
    {}
public:
    void StoreTextArchiveDocAttr(const TString& name, const TString& value) override {
        DocAttrs->AddAttr(name, value, TFullDocAttrs::AttrArcText);
    }
    void StoreFullArchiveDocAttr(const TString& name, const TString& value) override {
        DocAttrs->AddAttr(name, value, TFullDocAttrs::AttrArcFull);
    }
    void StoreErfDocAttr(const TString& name, const TString& value) override {
        DocAttrs->AddAttr(name, value, TFullDocAttrs::AttrErf);
    }
    void StoreGrpDocAttr(const TString& name, const TString& value, bool isInt) override {
        DocAttrs->AddAttr(name, value, isInt ? TFullDocAttrs::AttrGrInt : TFullDocAttrs::AttrGrName);
    }
};

}
