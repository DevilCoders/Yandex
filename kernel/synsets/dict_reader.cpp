#include "common.h"
#include "dict_reader.h"

TSynsetDictReader::TSynsetDictReader(const TBlob& data)
    : Data(data)
    , TrieData(GetBlock(Data, 1))
    , Trie(TrieData)
    , SynsetData(GetBlock(Data, 2))
    , Synset(SynsetData)
{
    TSingleValue<ui32> value( GetBlock(Data, 0) );
    if (1 != value.Get())
        ythrow yexception() << "bad version " << value.Get() << " of dict data in blob";
}

TSynsetDictReader::TSynsetDictReader(const TString& filename)
    : Data(TBlob::FromFileContent(filename))
    , TrieData(GetBlock(Data, 1))
    , Trie(TrieData)
    , SynsetData(GetBlock(Data, 2))
    , Synset(SynsetData)
{
    TSingleValue<ui32> value( GetBlock(Data, 0) );
    if (1 != value.Get())
        ythrow yexception() << "bad version " << value.Get() << " of '" << filename << "'";
}

void TSynsetDictReader::Fill(const TString& lemma, TSynset* result) const {
    ui64 value;
    if (Trie.Get(lemma.data(), &value)) {
        TInterval it(value);
        for (size_t i = it.Begin; i < it.End; ++i)
            result->push_back(Synset.Get(i));
    }
}
