#include "trie_data.h"

#include <library/cpp/charset/wide.h>

namespace NDistBetweenWords {
    static const TUtf16String DELIMITER = CharToWide("\v", CODES_ASCII);

    TUtf16String BuildKey(const TString& word1, const TString& word2) {
        return BuildKey(CharToWide(word1, CODES_UTF8), CharToWide(word2, CODES_UTF8));
    }

    TUtf16String BuildKey(const TString& word) {
        return BuildKey(CharToWide(word, CODES_UTF8));
    }

    TUtf16String BuildKey(const TUtf16String& word1, const TUtf16String& word2) {
        TUtf16String key(Reserve(word1.size() + word2.size() + 1));
        key.append(word1);
        key.append(DELIMITER);
        key.append(word2);
        return key;

    }

    TUtf16String BuildKey(const TUtf16String& word) {
        return DELIMITER + word;
    }

    IOutputStream& operator << (IOutputStream& stream, const TTrieData& data) {
        stream << "host5=" << data.Host5 << " host10=" << data.Host10 <<
               " Url5=" << data.Url5 << " Url10=" << data.Url10;
        return stream;
    }
}
