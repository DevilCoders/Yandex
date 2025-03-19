#include <library/cpp/on_disk/chunks/chunked_helpers.h>
#include <library/cpp/containers/comptrie/chunked_helpers_trie.h>

#include "common.h"
#include "dict_writer.h"

void TSynsetDictWriter::Add(const TString& lemma, const TString& synonym) {
    Data[lemma].push_back(synonym);
}

void TSynsetDictWriter::Write(const TString& filename) {
    TFixedBufferFileOutput fOut(filename);
    TChunkedDataWriter writer(fOut);

    TTrieMapWriter<ui64, false> trie;
    TStringsVectorG<true>::T data;

    for (TData::const_iterator toLemma = Data.begin(); toLemma != Data.end(); ++toLemma) {
        size_t begin = data.Size();
        for (TSynset::const_iterator toSyn = toLemma->second.begin(); toSyn != toLemma->second.end(); ++toSyn)
            data.PushBack(*toSyn);
        size_t end = data.Size();
        trie.Add(toLemma->first, TInterval(begin, end).Pack());
    }
    writer.NewBlock();
    TSingleValueWriter<ui32> versionWriter(1);
    versionWriter.Save(writer);
    writer.NewBlock();
    trie.Save(writer, true);
    writer.NewBlock();
    data.Save(writer);
    writer.WriteFooter();
}
