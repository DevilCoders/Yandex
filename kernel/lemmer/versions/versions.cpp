#include "versions.h"

#include <library/cpp/containers/comptrie/comptrie.h>
#include <util/generic/vector.h>
#include <library/cpp/langs/langs.h>
#include <util/generic/set.h>
#include <util/generic/string.h>
#include <util/stream/input.h>

namespace {
    struct TLanguageVersioning {
        using THash = TCompactTrie<wchar16, ui64, TNullPacker<ui64>>;
        THash Hash;
        void Initialize(IInputStream& stream) {
            Hash = THash(TBlob::FromStream(stream));
        }
        bool Has(const TUtf16String& word) {
            return Hash.Find(word.c_str());
        }
    };
}

namespace NLanguageVersioning {
    void InitializeHash(IInputStream& src, ELanguage) {
        return Singleton<TLanguageVersioning>()->Initialize(src);
    }

    bool IsWordHashed(const TUtf16String& word, ELanguage) {
        return Singleton<TLanguageVersioning>()->Has(word);
    }
}
