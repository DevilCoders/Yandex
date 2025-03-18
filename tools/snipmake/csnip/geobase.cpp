#include "geobase.h"

#include <kernel/groupattrs/metainfo.h>
#include <util/folder/dirut.h>

class TImagesGeobase::TImpl {
public:
    NGroupingAttrs::TMetainfo Metainfo;

    explicit TImpl(const TString& geoa)
        : Metainfo(false)
    {
        Metainfo.Scan(geoa.data(), NGroupingAttrs::TMetainfo::C2P);
    }

    inline bool Contains(TCateg biggerCateg, TCateg smallerCateg) const {
        return Metainfo.HasValueInCategPath(biggerCateg, smallerCateg);
    }

    inline TCateg Categ2Parent(TCateg categ) const {
        return Metainfo.Categ2Parent(categ);
    }
};

TImagesGeobase::TImagesGeobase(const TString& geoaFile)
{
    if (NFs::Exists(geoaFile)) {
        Impl.Reset(new TImpl(geoaFile));
    } else {
        ythrow yexception() << "Error: file" + geoaFile + " is missing.";
    }
}

TImagesGeobase::~TImagesGeobase() {
}

bool TImagesGeobase::Contains(TCateg biggerCateg, TCateg smallerCateg) const {
    if (biggerCateg == smallerCateg) {
        return true;
    }

    return Impl && Impl->Contains(biggerCateg, smallerCateg);
}

TCateg TImagesGeobase::Categ2Parent(TCateg categ) const {
    return Impl ? Impl->Categ2Parent(categ) : 0;
}
