#pragma once

#include <kernel/remorph/core/core.h>
#include <library/cpp/unicode/set/set.h>

#include <util/charset/wide.h>
#include <util/generic/ptr.h>
#include <util/generic/strbuf.h>
#include <util/generic/vector.h>
#include <util/generic/string.h>
#include <util/generic/hash.h>
#include <util/generic/bitmap.h>
#include <util/ysaveload.h>

namespace NReMorph {
namespace NPrivate {

struct TElement {
    TElement()
        : ExactChar(0)
    {
    }
    TElement(const NUnicode::TUnicodeSetPtr& charSet)
        : CharSet(charSet)
        , ExactChar(0)
    {
    }
    TElement(wchar32 c)
        : ExactChar(c)
    {
    }

    TString ToString() const {
        if (CharSet) {
            return CharSet->ToString();
        }
        char buff[4];
        size_t wl = 0;
        WideToUTF8(&ExactChar, 1, buff, wl);
        Y_ASSERT(wl <= Y_ARRAY_SIZE(buff));
        return TString(buff, wl);
    }


    void Save(IOutputStream* out) const {
        ::Save(out, ExactChar);
        if (CharSet) {
            Y_ASSERT(0 == ExactChar);
            CharSet->Save(out);
        }
    }
    void Load(IInputStream* in) {
        ::Load(in, ExactChar);
        if (0 == ExactChar) {
            CharSet = new NUnicode::TUnicodeSet();
            CharSet->Load(in);
        }
    }
    NUnicode::TUnicodeSetPtr CharSet;
    wchar32 ExactChar;
};

class TLiteralTableImpl: public TSimpleRefCount<TLiteralTableImpl> {
protected:
    typedef THashMap<TString, NRemorph::TLiteral> TTable;

protected:
    TVector<TElement> Literals;
    TTable Table;

protected:
    static TString ToString(wchar32 c) {
        char buff[4];
        size_t wl = 0;
        WideToUTF8(&c, 1, buff, wl);
        return TString(buff, wl);
    }

    NRemorph::TLiteral Add(const TElement& ce) {
        TString name = ce.ToString();
        TTable::iterator i = Table.find(name);
        if (i == Table.end()) {
            if (Literals.size() > NRemorph::TLiteral::MAX_ID)
                ythrow yexception() << "Too many literals";
            NRemorph::TLiteral l(Literals.size(), NRemorph::TLiteral::Ordinal);
            Literals.push_back(ce);
            Table.insert(std::make_pair(name, l));
            return l;
        }
        return i->second;
    }

public:
    inline NRemorph::TLiteral Add(const NUnicode::TUnicodeSetPtr& us) {
        return Add(TElement(us));
    }

    inline NRemorph::TLiteral Add(const NUnicode::TUnicodeSet& us) {

        return Add(NUnicode::TUnicodeSetPtr(new NUnicode::TUnicodeSet(us)));
    }

    inline NRemorph::TLiteral Add(wchar32 c) {
        return Add(TElement(c));
    }

    inline const TElement& Get(NRemorph::TLiteral l) const {
        Y_ASSERT(l.IsOrdinal());
        return Literals[l.GetId()];
    }

    inline bool IsEqual(NRemorph::TLiteral l, wchar32 c) const {
        Y_ASSERT(l.IsOrdinal());
        const TElement& ce = Get(l);
        return ce.CharSet ? ce.CharSet->Has(c) : ce.ExactChar == c;
    }

    inline TString ToString(NRemorph::TLiteral l) const {
        Y_ASSERT(l.IsOrdinal());
        return Get(l).ToString();
    }
    inline void Save(IOutputStream& out) const {
        ::Save(&out, Literals);
    }
    inline void Load(IInputStream& in) {
        ::Load(&in, Literals);
    }
    inline NRemorph::TLiteralId Size() const {
        return (NRemorph::TLiteralId)Literals.size();
    }
    inline NRemorph::TLiteralId GetPriority(NRemorph::TLiteral l) const {
        if (!Literals[l.GetId()].CharSet) {
            // simple chars have highest priority
            return 0;
        }
        // all the others have priority in order of appearance in rules file
        return l.GetId() + 1;
    }
};

typedef NRemorph::TLiteralTable<TLiteralTableImpl> TLiteralTable;

typedef TIntrusivePtr<TLiteralTable> TLiteralTablePtr;

} // NPrivate
} // NReMorph
