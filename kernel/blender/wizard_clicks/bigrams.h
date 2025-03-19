#pragma once

#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <util/string/split.h>

namespace NWizardsClicks {

    template <typename TCont>
    inline void Insert(TCont& cont, const typename TCont::value_type& value) {
        cont.insert(value);
    }

    template <>
    inline void Insert(TVector<TString>& cont, const TString& value) {
        cont.push_back(value);
    }

    template <typename TResult>
    void GetWords(TResult& cont, const TString& query) {
        TContainerConsumer<TResult> consumer(&cont);
        TSkipEmptyTokens< TContainerConsumer<TResult> > skipper(&consumer);
        SplitString(query.data(), query.data() + query.size(), TCharDelimiter<const char>(' '), skipper);
    }

    template <typename TResult>
    void GetBigrams(TResult& cont, const TString& query, bool addOneWord = true) {
        TVector<TString> words;
        GetWords(words, query);
        if (words.size() == 1 && addOneWord) {
            Insert(cont, words[0]);
        } else if (words.size() > 1) {
            for (size_t i = 0; i + 1 < words.size(); ++i) {
                Insert(cont, TString::Join(words[i], " ", words[i + 1]));
            }
        }
    }

}
