#include <library/cpp/deprecated/split/split_iterator.h>
#include <util/stream/file.h>

#include <library/cpp/on_disk/aho_corasick/writer.h>
#include <library/cpp/on_disk/chunks/writer.h>
#include <library/cpp/on_disk/chunks/chunked_helpers.h>
#include <library/cpp/containers/comptrie/chunked_helpers_trie.h>

#include "writer.h"

static void ReadPair(const TString& line, TString* s, i32* reg) {
    try {
        static const TSplitDelimiters DELIMS("\t");
        const TDelimitersSplit splitter(line, DELIMS);
        TDelimitersSplit::TIterator it = splitter.Iterator();
        const TString substr0 = it.NextString();
        *s = substr0;
        s->to_lower();
        *reg = FromString<i32>(it.NextString());
        if (!it.Eof())
            ythrow yexception() << "bad string";
    } catch (...) {
        ythrow yexception() << "error during parse '" << line << "': " << CurrentExceptionMessage();
    }
}

static void WriteRegTrie(const TString& filename, TChunkedDataWriter& writer) {
    try {
        TTrieMapWriter<i32> trieWriter;
        TFileInput fIn(filename);
        TString line;
        while (fIn.ReadLine(line)) {
            TString substr;
            i32 categ;
            ReadPair(line, &substr, &categ);
            trieWriter.Add(substr.data(), categ);
        }
        WriteBlock(writer, trieWriter);
    } catch (...) {
        ythrow yexception() << "error during parse '" << filename << "': " << CurrentExceptionMessage();
    }
}

static void WriteRegNumTrie(const TString& filename, TChunkedDataWriter& writer) {
    TTrieMapWriter<i32> trieWriter;
    TFileInput fIn(filename);
    TString line;
    while (fIn.ReadLine(line)) {
        TString substr;
        i32 categ;
        ReadPair(line, &substr, &categ);
        static const TString PREFIXES[] = {"r", "to", "tu", "git"};
        for (size_t i = 0; i < 4; ++i)
            trieWriter.Add((PREFIXES[i] + substr).data(), categ);
        if (substr[0] == '0') {
            substr = substr.substr(1);
            trieWriter.Add((TString("r") + substr).data(), categ);
        }
    }
    WriteBlock(writer, trieWriter);
}

void WriteGeoUrl(const TString& folder, IOutputStream* out) {
    TChunkedDataWriter writer(*out);

    {
        TSingleValueWriter<ui32> versionWriter(1);
        WriteBlock(writer, versionWriter);
    }

    WriteRegTrie(folder + "/regtlds.txt", writer);
    WriteRegTrie(folder + "/regdomains.txt", writer);
    WriteRegNumTrie(folder + "/regcodes.txt", writer);

    {
        TDefaultAhoCorasickBuilder ahoBuilder;
        TYVectorWriter<i32> index2reg;
        TYVectorWriter<i32> index2flags;
        TStringsVectorWriter substrs;
        TTrieMapWriter<i32> trieWriter;

        TString line;
        size_t index = 0;
        TFileInput fIn(folder + "/geourl.txt");
        while (fIn.ReadLine(line)) {
            TString substr;
            i32 reg;
            int flags;

            try {
                static const TSplitDelimiters DELIMS("\t");
                const TDelimitersSplit splitter(line, DELIMS);
                TDelimitersSplit::TIterator it = splitter.Iterator();
                const TString substr0 = it.NextString();
                substr = substr0;
                substr.to_lower();
                reg = FromString<i32>(it.NextString());
                flags = FromString<i32>(it.NextString());
                if (!it.Eof())
                    ythrow yexception() << "bad string";
            } catch (...) {
                ythrow yexception() << "error during parse '" << line << "': " << CurrentExceptionMessage();
            }

            ahoBuilder.AddString(substr, index);
            substrs.PushBack(substr);
            index2reg.PushBack(reg);
            index2flags.PushBack(flags);
            trieWriter.Add(substr.data(), index);
            ++index;
        }

        writer.NewBlock();
        ahoBuilder.SaveToStream(&writer);
        WriteBlock(writer, index2reg);
        WriteBlock(writer, index2flags);
        WriteBlock(writer, substrs);
        WriteBlock(writer, trieWriter);
    }

    writer.WriteFooter();
}
