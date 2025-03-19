#pragma once

#include <library/cpp/on_disk/aho_corasick/reader.h>
#include <library/cpp/on_disk/aho_corasick/writer.h>

#include <util/generic/string.h>

namespace NUrlCutter
{
    struct TQueryWord
    {
        TUtf16String Word;
        i32 RevFreq;
        size_t Len;
        bool IsStop;

        TQueryWord()
            : RevFreq(0)
            , Len(0)
            , IsStop(false)
        {
        }

        TQueryWord(const TUtf16String& word, i32 revFreq, size_t len, bool isStop)
            : Word(word)
            , RevFreq(revFreq)
            , Len(len)
            , IsStop(isStop)
        {
        }
    };
}

template<>
struct TSaveLoadVectorNonPodElement<NUrlCutter::TQueryWord> {
    typedef NUrlCutter::TQueryWord TItem;
    typedef wchar16 TCharType;
    static inline void Save(IOutputStream* out, const TItem& qw) {
        out->Write(qw.Word.data(), (qw.Word.size() + 1)*sizeof(TCharType));
        TSerializer<i32>::Save(out, qw.RevFreq);
        TSerializer<size_t>::Save(out, qw.Len);
        TSerializer<bool>::Save(out, qw.IsStop);
    }

    static inline void Load(TMemoryInput* in, TItem& qw, size_t elementSize) {
        static const size_t FIXED_FIELDS_SIZE = sizeof(i32) + sizeof(size_t) + sizeof(bool);
        Y_ASSERT(elementSize > FIXED_FIELDS_SIZE);
        size_t wordSize = elementSize - FIXED_FIELDS_SIZE;
        qw.Word.assign((const TCharType*)in->Buf(), wordSize/sizeof(TCharType) - 1); /// excluding 0 at the end
        in->Skip(wordSize);
        TSerializer<i32>::Load(in, qw.RevFreq);
        TSerializer<size_t>::Load(in, qw.Len);
        TSerializer<bool>::Load(in, qw.IsStop);
    }
};

namespace NUrlCutter {
    typedef TAhoCorasickBuilder<TUtf16String, TQueryWord> TMatcherBuilder;
    typedef TMappedAhoCorasick<TUtf16String, TQueryWord> TMatcherSearcher;
}
