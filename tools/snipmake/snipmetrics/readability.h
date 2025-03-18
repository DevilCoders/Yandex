#pragma once

#include <util/generic/hash.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <utility>

namespace NSnippets {

    template <class CharType = wchar16>
    struct TSymbolSet {
        typedef std::pair<CharType, CharType> TRange;
        TVector< TRange > Ranges;
        TSymbolSet& add(CharType a, CharType b) {
            Ranges.push_back(TRange(a,b));
            return *this;
        }
        bool Check(CharType c) const {
            for (typename TVector< TRange >::const_iterator i = Ranges.begin(); i != Ranges.end(); ++i)
                if (c >= i->first && c <= i->second)
                   return true;
            return false;
        }
    };

    class TSymbolsStat {
    public:
        typedef TSymbolSet<wchar16> TSetType;
        TSymbolsStat::TSetType& CodePage(const TString& name);
        int IsMatched(const TString& name) const;
        void Test(wchar16 c);
    private:
        struct TStat {
            TSetType Set;
            int IsMatched;
            TStat():IsMatched(0){}
        };
        THashMap<TString, TStat> Info;
    };

}

