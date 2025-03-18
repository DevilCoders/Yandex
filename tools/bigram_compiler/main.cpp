#include <dict/query_bigrams/all_freqs.h>
#include <dict/query_bigrams/all_freqs_packers.h>

#include <library/cpp/containers/comptrie/comptrie.h>
#include <library/cpp/getopt/small/last_getopt.h>

#include <util/charset/wide.h>
#include <util/folder/path.h>
#include <util/generic/utility.h>
#include <util/stream/file.h>
#include <util/string/split.h>
#include <util/system/env.h>
#include <util/system/getpid.h>
#include <util/system/tempfile.h>

using TBigramBuilder = TCompactTrieBuilder<ui32, TAllFreqs>;
using TKey = TVector<ui32>;
using TValue = TAllFreqs;

static const size_t MAX_KEY_SIZE = 2;

class TLineParser {
public:
    explicit TLineParser(const TBlob& wordTrie);

    bool ParseLine(const TString& line);

    const TVector<ui32>& GetKey() const {
        return Key;
    }

    const TValue& GetValue() const {
        return Value;
    }

private:
    using TWordTrie = TCompactTrie<wchar16, ui32>;

    TWordTrie Words;
    TKey Key;
    TValue Value;

    ui32 FindWordID(const TStringBuf& word) const;
};

TLineParser::TLineParser(const TBlob& wordTrie)
    : Words(wordTrie)
{
    Key.reserve(MAX_KEY_SIZE);
}

ui32 TLineParser::FindWordID(const TStringBuf& word) const {
    TUtf16String wWord = UTF8ToWide(word);
    return Words.Get(wWord);
}

bool TLineParser::ParseLine(const TString& line) {
    Key.clear();
    Value = TValue();
    TVector<TStringBuf> parts;
    Split(line.c_str(), "\t", parts);
    static const size_t minPartCount = BT_END + 1;
    static const size_t maxPartCount = BT_END + MAX_KEY_SIZE;
    if (parts.size() <  minPartCount || parts.size() > maxPartCount) {
        Cerr << "Bad line \"" << line << "\": expected between " << minPartCount <<
            " and " << maxPartCount << " parts, found " << parts.size() <<  Endl;
        return false;
    }
    const size_t valueStart = parts.size() - BT_END;
    for (size_t i = 0; i < valueStart; ++i) {
        Key.push_back(FindWordID(parts[i]));
    }
    for (size_t i = 0; i < BT_END; ++i) {
        Value[i] = FromString<double>(parts[i + valueStart]);
    }
    return true;
}

//-----------------------------------------------------------------------------------------
struct TOptions {
    TString WordsFile;
    TString InputFile;
    TString OutputFile;
};

static const char STD_INPUT[] = "-";

static TString GetTmpFileName() {
    TFsPath result(GetEnv("TMPDIR"));
    result /= "bigram_compiler.tmp." + ToString(GetPID());
    return result;
}

static void AddValue(const TLineParser& parser, TValue& unigramFreqs,
    TBigramBuilder& builder)
{
    const TKey& key = parser.GetKey();
    Y_ASSERT(key.size() <= MAX_KEY_SIZE);
    if (key.size() != 1) {
        builder.Add(key.begin() + 1, key.size() - 1, parser.GetValue());
    } else {
        unigramFreqs = parser.GetValue();
    }
}

static void FlushBuilder(TBigramBuilder& builder, TValue& unigramFreqs, TVector<ui32>& offsets, TFileOutput& result) {
    char buffer[(BT_END - 1) * (sizeof(ui64) + 1)];
    TUnigramFreqPacker packer;
    const size_t unigramSize = packer.MeasureLeaf(unigramFreqs);
    Y_ASSERT(unigramSize <= Y_ARRAY_SIZE(buffer));
    packer.PackLeaf(buffer, unigramFreqs, unigramSize);
    result.Write(buffer, unigramSize);
    const size_t trieSize = CompactTrieMinimizeAndMakeFastLayout(result, builder);
    builder.Clear();
    unigramFreqs = TValue();
    offsets.push_back(offsets.back() + unigramSize + trieSize);
}

static void PackToParts(const TOptions& options) {
    TBlob words = TBlob::FromFile(options.WordsFile);
    TLineParser parser(words);
    THolder<IInputStream> bigrams(options.InputFile != STD_INPUT ?
         new TIFStream(options.InputFile) :
         new TBufferedInput(&Cin));
    TString line;
    ui32 lastKey = 0;
    TVector<ui32> offsets(2, 0);
    if (!bigrams->ReadLine(line)) {
        return;
    }
    parser.ParseLine(line);
    lastKey = parser.GetKey().front();

    TBigramBuilder builder(CTBF_PREFIX_GROUPED | CTBF_UNIQUE);
    TValue unigramFreqs;
    AddValue(parser, unigramFreqs, builder);

    const TString bodyFile = GetTmpFileName();
    TTempFile bodyEraser(bodyFile);
    TFileOutput tries(bodyFile);

    while (bigrams->ReadLine(line) && parser.ParseLine(line)) {
        const ui32 key = parser.GetKey().front();
        if (key != lastKey) {
            if (key < lastKey) {
                ythrow yexception() << "Non-monotonous keys: " << key << " after " << lastKey;
            }
            FlushBuilder(builder, unigramFreqs, offsets, tries);
            lastKey = key;
            while (offsets.size() <= key) {
                offsets.push_back(offsets.back());
            }
        }
        AddValue(parser, unigramFreqs, builder);
    }
    FlushBuilder(builder, unigramFreqs, offsets, tries);
    if (offsets.size() != lastKey + 2) {
        ythrow yexception() << "Size mismatch: last key is " << lastKey << ", but there are only " << offsets.size() << "offsets";
    }
    tries.Finish();
    TFileOutput result(options.OutputFile);
    const ui32 version = 0;
    result.Write(&version, sizeof(version));
    const ui32 wordsSize = words.Length();
    result.Write(&wordsSize, sizeof(wordsSize));
    result.Write(words.Data(), words.Length());
    const ui32 tableSize = offsets.size();
    result.Write(&tableSize, sizeof(tableSize));
    result.Write(offsets.begin(), tableSize * sizeof(ui32));
    TIFStream body(bodyFile);
    TransferData(static_cast<IZeroCopyInput*>(&body), &result);
}

int main(int argc, char* argv[])
try {
    TOptions options;
    NLastGetopt::TOpts opts;
    opts.AddHelpOption();
    opts.AddLongOption('w', "words-trie").StoreResult(&options.WordsFile).
        RequiredArgument("TRIE_WITH_IDS").Required();
    opts.AddLongOption('o', "output").StoreResult(&options.OutputFile).
        RequiredArgument("FILE").Required();
    opts.AddLongOption('i', "input").StoreResult(&options.InputFile).
        RequiredArgument("FILE").DefaultValue(STD_INPUT).Optional();
    opts.SetFreeArgsMax(0);
    NLastGetopt::TOptsParseResult res(&opts, argc, argv);
    PackToParts(options);
    return 0;
} catch (const yexception& e) {
    Cerr << "Exception: " << e.what() << Endl;
    return 2;
}
