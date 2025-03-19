#include <assert.h>

#include <util/generic/string.h>

#include "trie.h"

#if TEST_TRIE
#define TEST_TRIE_OUT(a) a
#else
#define TEST_TRIE_OUT(a) ;
#endif

// Трай: Каждый узел лежит рядом со своими соседями, помнит ссылку на первого потомка
// и кол-во непосредственных потомков
// Список строк "abcx" "abcxy" "abcxyz" "abd" "abe" "abdf" "az" преобразуется к виду:
//    _  _
// ||| || |||
// |v| v| v|v
// 0abzcdefxyz
//  |^ |   ^|^
//  || |___|||

template <typename TChr>
typename TTrieCompiler<TChr>::TChildRange TTrieCompiler<TChr>::GenTrie(TTrie& trie, const TTrie& trTemp, size_t pos) {
    assert(pos < trTemp.size());
    TEST_TRIE_OUT(Cerr << "TTrieCompiler::TChildRange " << trTemp.size() << " " << pos << Endl);
    if (!trTemp[pos].Char) {
        TEST_TRIE_OUT(Cerr << "TTrieCompiler::GenTrie: returning to parent with " << 0 << " siblings (no node)" << Endl);
        return TChildRange(TRIE_NO_CHILD, 0);
    }
    const TTriePos This = (TTriePos)trie.size();
    trie.push_back(trTemp[pos]);
    TTrieChildsNum Num = 0;
    if (trTemp[pos].Childs.first != TRIE_NO_CHILD) {
        Num = GenTrie(trie, trTemp, trTemp[pos].Childs.first).second;
        TEST_TRIE_OUT(Cerr << "Node " << This << " has got " << Num << " siblings" << Endl << Endl);
    }

    // присвоение через временную переменную работает. Напрямую - не всегда. Don't ask me, why (gcc3.46)
    TChildRange r = GenTrie(trie, trTemp, pos + 1);
    trie[This].Childs = r;
    TEST_TRIE_OUT(Cerr << "TTrieCompiler::GenTrie: node " << This << " have got " << trie[This].Childs.second << " childs; the first one is " << trie[This].Childs.first << Endl << Endl);
    TEST_TRIE_OUT(Cerr << "TTrieCompiler::GenTrie: returning to parent with " << Num + 1 << " siblings (node " << This << ")" << Endl);
    return TChildRange(This, Num + 1);
}

template <typename TChr>
void TTrieCompiler<TChr>::GenTrieHeadInt(TTrie& trie, const TTrie& trTemp) {
    trie.clear();
    trie.push_back(0);
    // присвоение через временную переменную работает. Напрямую - не всегда. Don't ask me, why (gcc3.46)
    TChildRange r = GenTrie(trie, trTemp, 0);
    trie[0].Childs = r;
    TEST_TRIE_OUT(Cerr << "TTrieCompiler::GenTrieHead: node " << 0 << " have got " << trie[0].Childs.second << " childs; the first one is " << trie[0].Childs.first << Endl << Endl);
}

template <>
void TTrieCompiler<wchar16>::GenTrieHead(TTrie& trie, const TTrie& trTemp) {
    GenTrieHeadInt(trie, trTemp);
}

template <>
void TTrieCompiler<char>::GenTrieHead(TTrie& trie, const TTrie& trTemp) {
    GenTrieHeadInt(trie, trTemp);
}

template <typename TChr>
void TTrieCompiler<TChr>::DumpTempInt(IOutputStream& strm, const TTrie& trTemp, TStringType s, size_t pos) {
    if (!trTemp[pos].Char)
        return;
    if (trTemp[pos].Childs.first != TRIE_NO_CHILD)
        DumpTemp(strm, trTemp, s, trTemp[pos].Childs.first);
    s += trTemp[pos].Char;
    DumpTemp(strm, trTemp, s, pos + 1);
    if (trTemp[pos].Val != TRIE_NO_CHILD)
        strm << s << " " << trTemp[pos].Val << "\n";
}

template <>
void TTrieCompiler<wchar16>::DumpTemp(IOutputStream& strm, const TTrie& trTemp, TUtf16String s, size_t pos) {
    DumpTempInt(strm, trTemp, s, pos);
}

template <>
void TTrieCompiler<char>::DumpTemp(IOutputStream& strm, const TTrie& trTemp, TString s, size_t pos) {
    DumpTempInt(strm, trTemp, s, pos);
}

template <typename TChr>
void TTrieCompiler<TChr>::DumpTrieInt(IOutputStream& strm, const TTrie& trie, TStringType s, size_t pos) {
    if (trie[pos].Char)
        s += trie[pos].Char;
    if (trie[pos].Val != TRIE_NO_CHILD)
        strm << s << " " << trie[pos].Val << "\n";
    if (trie[pos].Childs.first == TRIE_NO_CHILD)
        return;
    for (size_t i = 0; i < trie[pos].Childs.second; ++i)
        DumpTrie(strm, trie, s, (size_t)trie[pos].Childs.first + i);
}

template <>
void TTrieCompiler<wchar16>::DumpTrie(IOutputStream& strm, const TTrie& trie, TUtf16String s, size_t pos) {
    DumpTrieInt(strm, trie, s, pos);
}

template <>
void TTrieCompiler<char>::DumpTrie(IOutputStream& strm, const TTrie& trie, TString s, size_t pos) {
    DumpTrieInt(strm, trie, s, pos);
}

//////////////////////////////////////////////////////////////////////////
// TStringTrieCreator

static const char* StrName = "st";

template <typename TChr>
static void WriteStr(IOutputStream& strm, const TBasicString<TChr>& str, size_t ind) {
    strm << "static const " << TTrieCompiler<TChr>::CharName() << " "
         << StrName << ind << "[] = {";
    size_t i = 0;
    for (; i < str.length(); ++i) {
        if (i)
            strm << ", ";
        strm << int(str[i]);
    }
    if (i)
        strm << ", ";
    strm << "0};\n";
}

template <typename TChr>
void TStringTrieCreator<TChr>::SaveToInt(IOutputStream& strm, const char* trieName, const char* dataName) {
    const size_t StrLn = 16;
    if (Val2Id.empty())
        return;

    TTrieCompiler<TChr>::SaveAsCpp(Key2Id.begin(), Key2Id.end(), strm, trieName);

    TVector<const TBasicString<TChr>*> v(Val2Id.size(), nullptr);
    for (typename TMapType::const_iterator i = Val2Id.begin(); i != Val2Id.end(); ++i)
        v[i->second] = &i->first;

    strm << Endl;
    for (size_t i = 0; i < v.size(); ++i)
        WriteStr<TChr>(strm, *v[i], i);
    strm << Endl;

    TString dataN = dataName ? dataName : "";
    if (dataN.empty()) {
        dataN = trieName;
        dataN += "Data";
    }
    strm << "const " << TTrieCompiler<TChr>::CharName() << " *" << dataN;
    strm << "[] = {\n    ";
    for (size_t i = 0; i < v.size(); ++i) {
        if (i) {
            strm << ",";
            if (i > 1 && i % StrLn == 1)
                strm << "\n    ";
            else
                strm << " ";
        }
        strm << StrName << i;
    }
    strm << "\n};\n";
}

template <>
void TStringTrieCreator<char>::SaveTo(IOutputStream& strm, const char* trieName, const char* dataName) {
    SaveToInt(strm, trieName, dataName);
}

template <>
void TStringTrieCreator<wchar16>::SaveTo(IOutputStream& strm, const char* trieName, const char* dataName) {
    SaveToInt(strm, trieName, dataName);
}
