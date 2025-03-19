#include "docattrs.h"

#include <util/ysaveload.h>

void TFullDocAttrs::TAttr::Load(IInputStream* in) {
    ::Load(in, Name);
    ::Load(in, Value);
    ::Load(in, Type);
}

void TFullDocAttrs::PackTo(IOutputStream& out, ui32 mask, bool packAttrType) const {
    ui32 s = 0;
    for (TAttrs::const_iterator i = Attrs.begin(); i != Attrs.end(); ++i) {
        if ((*i).Type & mask) {
            ++s;
        }
    }
    ::Save(&out, s);

    for (TAttrs::const_iterator i = Attrs.begin(); i != Attrs.end(); ++i) {
        if ((*i).Type & mask) {
            ::Save(&out, (*i).Name);
            ::Save(&out, (*i).Value);
            if (packAttrType)
                ::Save(&out, i->Type);
        }
    }
}

void TFullDocAttrs::UnpackFrom(IInputStream& in) {
   Load(&in, Attrs);
}

void UnpackFrom(IInputStream* in, THashMap<TString, TString>* out) {
    Load(in, *out);
}

