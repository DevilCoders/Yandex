#include <kernel/snippets/custom/hostnames_data/hostnames.h>
#include <library/cpp/archive/yarchive.h>

namespace {
    static const unsigned char TRIES_DATA[] = {
        #include <kernel/snippets/custom/hostnames_data/hostnames.inc>
    };

    template <size_t N>
    THashMap<TString, NSnippets::W2WTrie> LoadTries(const unsigned char (&data)[N]) {
            TArchiveReader archive(TBlob::NoCopy(data, sizeof(data)));
            THashMap<TString, NSnippets::W2WTrie> result;
            for (size_t i = 0; i < archive.Count(); ++i) {
                const TString key = archive.KeyByIndex(i);
                TAutoPtr<IInputStream> trie = archive.ObjectByKey(key);
                result[key] = NSnippets::W2WTrie(TBlob::FromStream(*trie));
            }
            return result;
    }
};

namespace NSnippets {
    const THashMap<TString, W2WTrie> DOMAIN_SUBSTITUTE_TRIES = LoadTries(TRIES_DATA);
};
