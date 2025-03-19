#pragma once

#include <cstdio>

#include <util/generic/map.h>
#include <util/generic/vector.h>
#include <util/generic/string.h>
#include <util/generic/typetraits.h>
#include <util/stream/output.h>

using TTriePos = ui32;
using TTrieChildsNum = unsigned char;
static const TTriePos TRIE_NO_CHILD = (TTriePos)-1;

#define TEST_TRIE 0

template <typename TVal>
struct TTTrieFinderRet {
    TVal Val;
    size_t Depth;
    TTTrieFinderRet(const TVal& val, size_t depth)
        : Val(val)
        , Depth(depth)
    {
    }
};

using TTrieFinderRet = TTTrieFinderRet<TTriePos>;
using TStringTrieFinderRet = TTTrieFinderRet<const char*>;
using TWtringTrieFinderRet = TTTrieFinderRet<const wchar16*>;

#if defined(_compiler_msvc_)
#pragma pack(push)
#pragma pack(1)
#endif
template <typename TChr>
struct Y_PACKED TTrieNode {
    std::make_unsigned_t<TChr> Char;
    TTriePos Kids;
    TTrieChildsNum KidsNum;
    TTriePos Ref;
};
#if defined(_compiler_msvc_)
#pragma pack(pop)
#endif

template <typename TChr>
class TTrieCompiler {
    using TUChr = std::make_unsigned_t<TChr>;

public:
    typedef TBasicString<TChr> TStringType;
    typedef std::pair<TTriePos, TTrieChildsNum> TChildRange;
    struct TTrInt {
        TUChr Char;
        TChildRange Childs;
        TTriePos Val;
        TTrInt(TUChr c)
            : Char(c)
            , Childs(TRIE_NO_CHILD, 0)
            , Val(TRIE_NO_CHILD){};
    };
    typedef TVector<TTrInt> TTrie;

private:
    static long get_beg(const TChr* s1, const TChr* s2) {
        long i = 0;
        while (*s1 && *s1 == *s2)
            ++s1, ++s2, ++i;
        return *s1 ? -1 : i;
    }

    static TChildRange GenTrie(TTrie& trie, const TTrie& trTemp, size_t pos);
    static void GenTrieHead(TTrie& trie, const TTrie& trTemp);
    static void GenTrieHeadInt(TTrie& trie, const TTrie& trTemp);
    static void DumpTemp(IOutputStream& strm, const TTrie& trTemp, TStringType s = TStringType(), size_t pos = 0);
    static void DumpTrie(IOutputStream& strm, const TTrie& trie, TStringType s = TStringType(), size_t pos = 0);
    static void DumpTempInt(IOutputStream& strm, const TTrie& trTemp, TStringType s = TStringType(), size_t pos = 0);
    static void DumpTrieInt(IOutputStream& strm, const TTrie& trie, TStringType s = TStringType(), size_t pos = 0);

    template <class TInputIterator>
    static void CompileTrie(TInputIterator first, TInputIterator last, TTrie& trie);

    template <class TInputIterator>
    static void GenTempTr(TInputIterator first, TInputIterator last, TTrie& trTemp);

    static const char* CharName(char) {
        return "char";
    }
    static const char* CharName(wchar16) {
        return "wchar16";
    }

public:
    template <class TInputIterator>
    static void SaveAsCpp(TInputIterator first, TInputIterator last, IOutputStream& strm, const char* trieName);
    template <class TInputIterator>
    static void SaveBinary(TInputIterator first, TInputIterator last, FILE* f);

    template <class TInputIterator, class TCnt>
    static void SaveAsTrieNodes(TInputIterator first, TInputIterator last, TCnt& trNodes);
    static const char* CharName() {
        return CharName(TChr(0));
    }
};

template <typename TChr>
void LookFor(const TTrieNode<TChr> trie[], const TChr* str, TVector<TTrieFinderRet>& ret);
template <typename TChr>
TTrieFinderRet LookFor(const TTrieNode<TChr> trie[], const TChr* str);
template <typename TChr>
inline bool IsInTrie(const TTrieNode<TChr> trie[], const TChr* str) {
    return !str[LookFor(trie, str).Depth];
};

template <typename TChr>
template <class TInputIterator>
void TTrieCompiler<TChr>::CompileTrie(TInputIterator first, TInputIterator last, TTrie& trie) {
    TTrie trTemp;
    GenTempTr(first, last, trTemp);
#if TEST_TRIE
    Cout << "\nTemp trie generated\n";
#endif
    GenTrieHead(trie, trTemp);
#if TEST_TRIE
    Cout << "\nTrie generated\n";
#endif
#if TEST_TRIE
    DumpTemp(Cerr, trTemp);
    Cerr << "\n";
    DumpTrie(Cerr, trie);
#endif
}

// Промежуточный трай: Каждый узел режит рядом со своим потомком и хранит ссылку на соседа.
// Список строк "abcx" "abcxy" "abcxyz" "abd" "abdf" "abe" "az" преобразуется к виду (0 - chr(0)):
//   __________
//  | ____     |
//  ||    |    |
//  ||    V    V
// abcxyz0df0e0z0
//        |  ^
//        |__|

// !!!Ключи на входе должны быть отсортированны по алфавиту!!!
template <typename TChr>
template <class TInputIterator>
void TTrieCompiler<TChr>::GenTempTr(TInputIterator first, TInputIterator last, TTrie& trTemp) {
    typedef std::vector<TTriePos> TVDist;
    trTemp.clear();
    TVDist vpos;
    TTriePos cur_pos = TRIE_NO_CHILD;
    TStringType currS;

    while (first != last) {
        while (get_beg(currS.c_str(), first->first.c_str()) < 0) {
            cur_pos = vpos.back();
            vpos.pop_back();
            currS.resize(currS.length() - 1);
        }
        if (cur_pos != TRIE_NO_CHILD)
            trTemp[cur_pos].Childs.first = (TTriePos)trTemp.size();

        long pos = 0;
        while (first != last && (pos = get_beg(currS.c_str(), first->first.c_str())) >= 0) {
            TChr c = 0;
            for (size_t i = (size_t)pos; (c = first->first[i]); ++i) {
                trTemp.push_back(c);
                currS += c;
                vpos.push_back((TTriePos)trTemp.size() - 1);
            }
            assert(!trTemp.empty());
            trTemp.back().Val = first->second;
#ifndef NDEBUG
            TStringType sDebug = first->first;
#endif
            ++first;
            assert(first == last || first->first > sDebug);
        }
        trTemp.push_back(0);
    }
}

// !!!Ключи на входе должны быть отсортированны по алфавиту!!!
template <typename TChr>
template <class TInputIterator>
void TTrieCompiler<TChr>::SaveAsCpp(TInputIterator first, TInputIterator last, IOutputStream& strm, const char* trieName) {
    TTrie trie;
    CompileTrie(first, last, trie);
    strm << "#include <kernel/lemmer/untranslit/trie/trie.h>\n\n";
    strm << "TTrieNode<" << CharName() << "> " << trieName << "[] = {\n";
    for (size_t i = 0; i < trie.size(); ++i)
        strm << "    {" << (int)trie[i].Char << ", "
             << trie[i].Childs.first << "U, "
             << (TTriePos)trie[i].Childs.second << "U, "
             << trie[i].Val << "U},        //" << i << "\n";
    strm << "};\n";
}

// !!!Ключи на входе должны быть отсортированны по алфавиту!!!
template <typename TChr>
template <class TInputIterator>
void TTrieCompiler<TChr>::SaveBinary(TInputIterator first, TInputIterator last, FILE* f) {
    TTrie trie;
    CompileTrie(first, last, trie);
    TTriePos t = (TTriePos)trie.size();
    fwrite(&t, sizeof(t), 1, f);
    unsigned char buf[sizeof(TTrieNode<TChr>)];
    for (size_t i = 0; i < trie.size(); ++i) {
        memcpy(buf, &trie[i].Char, sizeof(trie[i].Char));
        memcpy(buf + sizeof(trie[i].Char), &trie[i].Childs.first, sizeof(trie[i].Childs.first));
        memcpy(buf + sizeof(trie[i].Char) + sizeof(trie[i].Childs.first), &trie[i].Childs.second, sizeof(trie[i].Childs.second));
        memcpy(buf + sizeof(trie[i].Char) + sizeof(trie[i].Childs.first) + sizeof(trie[i].Childs.second), &trie[i].Val, sizeof(trie[i].Val));
        fwrite(buf, 1, sizeof(trie[i].Char) + sizeof(trie[i].Childs.first) + sizeof(trie[i].Childs.second) + sizeof(trie[i].Val), f);
    }
}

// !!!Ключи на входе должны быть отсортированны по алфавиту!!!
template <typename TChr>
template <class TInputIterator, class TCnt>
void TTrieCompiler<TChr>::SaveAsTrieNodes(TInputIterator first, TInputIterator last, TCnt& trNodes) {
    TTrie trie;
    CompileTrie(first, last, trie);

    trNodes.resize(trie.size());

    for (size_t i = 0; i < trie.size(); ++i) {
        trNodes[i].Char = trie[i].Char;
        trNodes[i].Kids = trie[i].Childs.first;
        trNodes[i].KidsNum = trie[i].Childs.second;
        trNodes[i].Ref = trie[i].Val;
    }
}

//////////////////////////////////////////////////////////////////////////
// StringTrie
template <typename TChr>
class TStringTrieCreator {
    typedef TBasicString<TChr> TStringType;
    typedef TMap<TStringType, TTriePos> TMapType;
    TMapType Val2Id;
    TMapType Key2Id;

public:
    void Add(const TStringType& key, const TStringType& val) {
        typename TMapType::const_iterator i = Val2Id.find(val);
        if (i != Val2Id.end()) {
            Key2Id[key] = i->second;
            return;
        }
        TTriePos l = (TTriePos)Val2Id.size();
        Val2Id[val] = l;
        Key2Id[key] = l;
    }
    void Clear() {
        Val2Id.clear();
        Key2Id.clear();
    }
    void SaveTo(IOutputStream& strm, const char* trieName, const char* dataName = nullptr);

private:
    void SaveToInt(IOutputStream& strm, const char* trieName, const char* dataName = nullptr);
};

template <typename TChr>
void LookFor(const TTrieNode<TChr> trie[], const TChr* TrTestData[], const TChr* str, TVector<TTTrieFinderRet<const TChr*>>& ret);
template <typename TChr>
TTTrieFinderRet<const TChr*> LookFor(const TTrieNode<TChr> trie[], const TChr* TrTestData[], const TChr* str);
