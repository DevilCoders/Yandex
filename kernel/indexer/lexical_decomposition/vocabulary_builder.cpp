#include <library/cpp/charset/doccodes.h>
#include <library/cpp/langs/langs.h>
#include <library/cpp/charset/wide.h>
#include <util/stream/buffer.h>
#include <util/stream/output.h>
#include <library/cpp/deprecated/split/split_iterator.h>
#include <util/generic/hash_set.h>

#include <library/cpp/on_disk/aho_corasick/reader.h>
#include <library/cpp/on_disk/chunks/chunked_helpers.h>

#include "vocabulary_builder.h"

using namespace NLexicalDecomposition;

bool TSpecifiedVocabularyBuilder::InsertWord(const TUtf16String& word, const TWordAdditionalInfo& value) {
    ui32 id;
    if (word.empty() || Word2IdBuilder.Find(word.data(), word.size(), &id)) {
        return false;
    }
    id = Size();
    Word2IdBuilder.Add(word.data(), word.size(), id);
    AhoBuilder.AddString(word, id);
    Id2InfoBuilder.PushBack(value);
    return true;
}

void TSpecifiedVocabularyBuilder::Save(IOutputStream& output) {
    TChunkedDataWriter writer(output);

    writer.NewBlock();
    Word2IdBuilder.Save(writer);

    writer.NewBlock();
    Id2InfoBuilder.Save(writer);

    writer.NewBlock();
    TBlob blob = AhoBuilder.Save();
    writer.Write(blob.Data(), blob.Size());
    {
        TMappedAhoCorasick<TUtf16String, ui32, TMappedSingleOutputContainer<ui32>> checker(blob);
        checker.CheckData();
    }

    writer.WriteFooter();
}

TMultiVocabularyBuilder::TMultiVocabularyBuilder(bool verbose)
    : Verbose(verbose)
{
}

void TMultiVocabularyBuilder::LoadVocabulary(ELanguage language, IInputStream& input, ECharset charset) {
    if (Lang2Voc.count(language) == 0) {
        Lang2Voc[language] = TVocPtr(new TSpecifiedVocabularyBuilder());
    }
    if (Verbose) {
        Cerr << "Loading vocabulary, language: " << NameByLanguage(language) << Endl;
    }
    TString line;
    size_t counter = 0;

    while (input.ReadLine(line)) {
        ++counter;
        try {
            const static TSplitDelimiters DELIMS(" \t");
            const TDelimitersSplit split(line, DELIMS);
            TDelimitersSplit::TIterator it = split.Iterator();
            const TString word = it.NextString();

            ui32 freq = it.Eof() ? 0 : FromString<ui32>(it.NextString());
            EVocabularyWordType type = it.Eof() ? VWT_UNKNOWN : static_cast<EVocabularyWordType>(FromString<ui32>(it.NextString()));
            if (type != VWT_UNKNOWN && freq == 0) {
                ythrow yexception() << "zero frequency in the voc" << Endl;
            }
            if (type == VWT_STOP) {
                freq = 0;
            }

            TUtf16String wword = CharToWide(word, charset);
            Voc(language).InsertWord(wword, TWordAdditionalInfo(wword.length(), freq, type));
        } catch (const yexception& e) {
            Cerr << "Failed to process line #" << counter << ", message: " << e.what() << Endl;
        }
        if (Verbose && counter % 100000 == 0) {
            Cerr << counter / 1e+6 << " millions processed" << Endl;
        }
    }

    if (Verbose) {
        Cerr << "...done" << Endl;
    }
}

void TMultiVocabularyBuilder::LoadVocabulary(ELanguage language, const TString& filename, ECharset charset) {
    TFileInput input(filename);
    LoadVocabulary(language, input, charset);
}

void TMultiVocabularyBuilder::Save(IOutputStream& output) const {
    TNamedChunkedDataWriter writer(output);
    {
        writer.NewBlock("Version");
        writer.WriteBinary<ui32>(VOCABULARY_VERSION);
    }
    for (const auto& it : Lang2Voc) {
        writer.NewBlock(NameByLanguage(it.first));
        it.second->Save(writer);
    }
    writer.WriteFooter();
}

const TBlob TMultiVocabularyBuilder::Save() const {
    TBufferStream stream;
    Save(stream);
    return TBlob::FromStream(stream);
}

void TMultiVocabularyBuilder::Update(const TBlob& input, IOutputStream& output) const {
    TNamedChunkedDataWriter writer(output);

    {
        TNamedChunkedDataReader reader(input);

        ui32 inputVersion = TSingleValue<ui32>(reader.GetBlobByName("Version")).Get();
        if (inputVersion != VOCABULARY_VERSION)
            ythrow yexception() << "Unknown version: " << inputVersion << " instead of " << VOCABULARY_VERSION;

        THashSet<TString> existingBlocks;
        for (const auto& it : Lang2Voc)
            existingBlocks.insert(NameByLanguage(it.first));

        size_t inputSize = reader.GetBlocksCount();
        for (size_t i = 0; i < inputSize; i++)
            if (!existingBlocks.contains(reader.GetBlockName(i))) {
                writer.NewBlock(reader.GetBlockName(i));
                writer.Write(reader.GetBlock(i), reader.GetBlockLen(i));
            }
    }

    for (const auto& it : Lang2Voc) {
        writer.NewBlock(NameByLanguage(it.first));
        it.second->Save(writer);
    }

    writer.WriteFooter();
}

TSpecifiedVocabularyBuilder& TMultiVocabularyBuilder::Voc(ELanguage language) {
    Y_ASSERT(Lang2Voc.count(language) != 0);
    return *(Lang2Voc[language]);
}
