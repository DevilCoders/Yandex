#pragma once

#include <library/cpp/containers/comptrie/comptrie.h>
#include <util/generic/hash.h>
#include <util/ysaveload.h>
#include <util/charset/wide.h>

namespace NProgRulePrivate {

class TAlphabet {
    friend class TAlphabetBuilder;
public:
    struct TSymbol {
        ui32 Data;
        TSymbol(ui32 d)
            : Data(d) {
        }
        TSymbol()
            : Data(0) {
        }
        operator ui32() {
            return Data;
        }
        ui32 operator++() {
            return ++Data;
        }
        ui32 operator++(int) {
            ui32 retval = Data;
            operator++();
            return retval;
        }
    };
private:
    typedef THashMap<TUtf16String, TSymbol> TMapType;
    TMapType Map;
public:
    TAlphabet() {
    }
    TAlphabet(const TString& path) {
        TIFStream in(path);
        Load(in);
    }
    bool Has(const TUtf16String& w) const {
        return Map.contains(w);
    }
    TSymbol Get(const TUtf16String& w) const {
        TMapType::const_iterator it = Map.find(w);
        if (it == Map.end()) {
            ythrow yexception() << "word '" << w << "' not found";
        }
        return it->second;
    }
    void Save(IOutputStream& out) const {
        ::Save(&out, Map);
    }
    void Load(IInputStream& in) {
        ::Load(&in, Map);
    }
};

typedef TCompactTrie<TAlphabet::TSymbol, ui32> TTrie;

} // NProgRulePrivate

Y_DECLARE_PODTYPE(NProgRulePrivate::TAlphabet::TSymbol);
