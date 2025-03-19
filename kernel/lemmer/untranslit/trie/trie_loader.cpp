#include "trie.h"

template <class T>
class TOneValue {
public:
    T Val;
    TOneValue(const T& v)
        : Val(v){};
    void push_back(const T& v) {
        Val = v;
    }
};

//выдрано с мясом из БЫКа.
template <typename TChr, class T>
static void LookForInt(const TTrieNode<TChr> trie[], const TChr* str, T& ret) {
    size_t len = 0;
    wchar16 wordlet = '\x00';

    for (TTriePos i = trie[0].Kids, n = trie[0].KidsNum; n && (wordlet = str[len]); len++) {
        // binary search in this level
        TTriePos last = i + n;
        while (n > 0) {
            TTriePos half = n / 2;
            TTriePos probe = i + half;
            if (trie[probe].Char < wordlet) {
                i = probe + 1;
                n = n - half - 1;
            } else {
                n = half;
            }
        }
        if (i == last || trie[i].Char != wordlet)
            break;
        if (const auto ref = trie[i].Ref; ref != TRIE_NO_CHILD)
            ret.push_back(TTrieFinderRet(ref, len + 1));
        n = trie[i].KidsNum;
        i = trie[i].Kids;
    }
}

template <>
TTrieFinderRet LookFor<wchar16>(const TTrieNode<wchar16> trie[], const wchar16* str) {
    TOneValue<TTrieFinderRet> ret(TTrieFinderRet(TRIE_NO_CHILD, 0));
    LookForInt(trie, str, ret);
    return ret.Val;
}

template <>
TTrieFinderRet LookFor<char>(const TTrieNode<char> trie[], const char* str) {
    TOneValue<TTrieFinderRet> ret(TTrieFinderRet(TRIE_NO_CHILD, 0));
    LookForInt(trie, str, ret);
    return ret.Val;
}

template <>
void LookFor<wchar16>(const TTrieNode<wchar16> trie[], const wchar16* str, TVector<TTrieFinderRet>& ret) {
    ret.clear();
    LookForInt(trie, str, ret);
}

template <>
void LookFor<char>(const TTrieNode<char> trie[], const char* str, TVector<TTrieFinderRet>& ret) {
    ret.clear();
    LookForInt(trie, str, ret);
}

//////////////////////////////////////////////////////////////////////////
//StringTrie

template <typename TChr>
static void LookForString(const TTrieNode<TChr> trie[], const TChr* TrTestData[], const TChr* str, TVector<TTTrieFinderRet<const TChr*>>& ret) {
    TVector<TTrieFinderRet> r;
    LookFor(trie, str, r);
    ret.clear();
    for (size_t i = 0; i < r.size(); ++i)
        ret.push_back(TTTrieFinderRet<const TChr*>(TrTestData[r[i].Val], r[i].Depth));
}

template <>
void LookFor<char>(const TTrieNode<char> trie[], const char* TrTestData[], const char* str, TVector<TStringTrieFinderRet>& ret) {
    LookForString(trie, TrTestData, str, ret);
}

template <>
void LookFor<wchar16>(const TTrieNode<wchar16> trie[], const wchar16* TrTestData[], const wchar16* str, TVector<TWtringTrieFinderRet>& ret) {
    LookForString(trie, TrTestData, str, ret);
}

template <>
TStringTrieFinderRet LookFor<char>(const TTrieNode<char> trie[], const char* TrTestData[], const char* str) {
    TTrieFinderRet r = LookFor(trie, str);
    return TStringTrieFinderRet(r.Val != TRIE_NO_CHILD ? TrTestData[r.Val] : nullptr, r.Depth);
}

template <>
TWtringTrieFinderRet LookFor<wchar16>(const TTrieNode<wchar16> trie[], const wchar16* TrTestData[], const wchar16* str) {
    TTrieFinderRet r = LookFor(trie, str);
    return TWtringTrieFinderRet(r.Val != TRIE_NO_CHILD ? TrTestData[r.Val] : nullptr, r.Depth);
}
