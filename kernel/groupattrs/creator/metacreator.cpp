#include "metacreator.h"

#include <kernel/groupattrs/config.h>

#include <util/folder/dirut.h>
#include <util/stream/file.h>
#include <util/string/printf.h>

namespace NGroupingAttrs {

void IMetaInfoCreator::ParseFile(const TString& dir, const char* attrname) {
    Data[attrname].Reset(new TMetainfo(true));
    TMetainfo& mi = *(Data[attrname].Get());

    TString filename = TString::Join(dir.data(), "/", attrname);

    mi.LoadC2N(filename, attrname);
}

void TMetainfoCreator::LoadC2N(const TConfig& config, const TString& prefix) {
    const TString& dir = GetDirName(prefix);

    for (ui32 attrnum = 0; attrnum < config.AttrCount(); ++attrnum) {
        if (config.IsAttrNamed(attrnum)) {
            ParseFile(dir, config.AttrName(attrnum));
        }
    }
}

void TMetainfoCreator::SaveC2N(const TString& prefix) const {
    const TString& dir = GetDirName(prefix);

    THolder<TFixedBufferFileOutput> out;
    for (TData::const_iterator it = Data.begin(); it != Data.end(); ++it) {
        out.Reset(new TFixedBufferFileOutput(Sprintf("%s/%s.c2n", dir.data(), it->first.data())));
        const TMetainfo* meta = it->second.Get();
        for (TCateg c = 1; c <= (TCateg)meta->CategCount(); ++c) {
            const char* name = meta->Categ2Name(c);
            if (strcmp(name, NONAME) == 0) {
                ythrow yexception() << "Name for categ " << c << " is abscent";
            }
            out->Write(Sprintf("%d\t%s\n", static_cast<i32>(c), name).data());
        }
    }
}

TMetainfo* IMetaInfoCreator::Metainfo(const char* attrname) {
    if (!Data[attrname]) {
        Data[attrname].Reset(new TMetainfo(true));
    }

    return Data[attrname].Get();
}

TCateg TMetainfoCreator::AddCateg(const char* attrname, const char* categname) {
    TMetainfo* meta = Metainfo(attrname);
    return GetDynamicClassNames(*meta)->AddCateg(categname);
}

}
