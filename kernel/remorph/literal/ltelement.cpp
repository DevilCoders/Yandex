#include "ltelement.h"

namespace NLiteral {

void TLTElementBase::Save(IOutputStream* out) const {
    ::Save(out, (ui8)Type);
    ::Save(out, ExpId);
    ::Save(out, Name);
    switch (Type) {
#define X(A) case TLTElementBase::A: static_cast<const TLTElement##A*>(this)->Save(out); break;
        NRM_LT_ELEMENTS
#undef X
    }
}

TLTElementPtr TLTElementBase::Load(IInputStream* in) {
    ui8 type = 0;
    TExpressionId expId = Max<TExpressionId>();
    TString name;
    ::Load(in, type);
    ::Load(in, expId);
    ::Load(in, name);

    switch (type) {
#define X(A) case TLTElementBase::A: { \
        TAutoPtr<TLTElement##A> ptr(new TLTElement##A(name)); \
        ptr->Load(in); \
        ptr->ExpId = expId; \
        return ptr.Release(); \
    }
        NRM_LT_ELEMENTS
#undef X
    }
    return TLTElementPtr();
}

void ReadSet(const TString& path, TSetWtroka& elements) {
    TIFStream input(path);
    TString line;
    while (input.ReadLine(line)) {
        elements.insert(UTF8ToWide(line));
    }
}

TLTElementSetPtr CreateClassLiteral(const TString& path) {
    TLTElementSetPtr e(new TLTElementSet("class%" + path));
    ReadSet(path, e->TextSet);
    return e.Get();
}

TLTElementLemmaSetPtr CreateLemmaLiteral(const TString& path) {
    TLTElementLemmaSetPtr e(new TLTElementLemmaSet("class*%" + path));
    ReadSet(path, e->LemmaSet);
    return e.Get();
}

} // NLiteral
