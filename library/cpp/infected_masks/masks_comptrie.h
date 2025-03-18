#pragma once

#include <kernel/hosts/owner/owner.h>
#include <library/cpp/containers/comptrie/comptrie_trie.h>
#include <util/memory/blob.h>

class TCategMaskComptrie {
public:
    using TResult = TVector<std::pair<TString, TStringBuf>>;
    using TTrie = TCompactTrie<char, TStringBuf>;

private:
    TTrie Trie;
    TBlob Data;
    TOwnerCanonizer OwnerCanonizer;

private:
    void FindPath(TString& host, TStringBuf& urlPath, TResult& values, TTrie root) const;
    void Find(TTrie categSubtrie, TStringBuf hostname, TStringBuf urlPath, TResult& values) const;

public:
    TCategMaskComptrie();

    void Init(const TBlob& blob);
    bool Find(const TStringBuf& key, TResult& values) const;
    bool FindExact(const TStringBuf& key, TStringBuf& value) const;
    bool FindByUrl(const TStringBuf& url, TResult& values) const; // Makes key from given url and forwards it to Find

    TCompactTrie<char, TStringBuf> FindTails(TStringBuf prefix) const {
        return Trie.FindTails(prefix);
    }

    TCompactTrie<char, TStringBuf>::TConstIterator Begin() const {
        return Trie.Begin();
    }

    TCompactTrie<char, TStringBuf>::TConstIterator End() const {
        return Trie.End();
    }

    static TString GetInfectedMaskFromInternalKey(TStringBuf key);
};
