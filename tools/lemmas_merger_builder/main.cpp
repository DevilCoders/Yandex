#include <kernel/lemmas_merger/lemmas_merger.h>

#include <library/cpp/getopt/last_getopt.h>

#include <util/charset/wide.h>

#include <util/string/join.h>

#include <util/generic/string.h>
#include <util/generic/vector.h>

enum ERunMode {
    Build,
    Words,
    Hashes,
};

int main(int argc, const char **argv) {
    ERunMode runMode = Build;

    TVector<TString> dictionaryFileNames;
    TVector<TString> derivationsFileNames;
    TVector<TString> languageNames;

    TString lemmasMergerIndexFileName;

    size_t wordsCountLimit = 0;

    size_t textFieldNumber = (size_t) -1;

    try {
        NLastGetopt::TOpts opts;

        opts.AddLongOption("build", "build mode")
            .StoreValue(&runMode, Build).NoArgument();
        opts.AddLongOption("words", "read words mode")
            .StoreValue(&runMode, Words).NoArgument();
        opts.AddLongOption("hashes", "read hahses mode")
            .StoreValue(&runMode, Hashes).NoArgument();

        opts.AddLongOption("lm", "lemmas merger index file name")
            .Required()
            .StoreResult(&lemmasMergerIndexFileName);

        opts.AddLongOption("limit", "words count limit")
            .Optional()
            .StoreResult(&wordsCountLimit);

        opts.AddLongOption("field", "text field number in tab-separated input")
            .Optional()
            .StoreResult(&textFieldNumber);

        opts.AddLongOption("dictionaries", "dictionaries list (comma separated)")
            .Optional()
            .SplitHandler(&dictionaryFileNames, ',');
        opts.AddLongOption("derivations", "derivations list (comma separated)")
            .Optional()
            .SplitHandler(&derivationsFileNames, ',');
        opts.AddLongOption("languages", "language names list (comma separated)")
            .Optional()
            .SplitHandler(&languageNames, ',');

        opts.SetFreeArgsMax(0);

        NLastGetopt::TOptsParseResult res(&opts, argc, argv);
    } catch (yexception& ex) {
        Cerr << "Error parsing command line options: " << ex.what() << "\n";
        return 1;
    }

    if (runMode == Build) {
        NLemmasMerger::TLemmasMerger lemmasMerger;
        lemmasMerger.Build(languageNames, dictionaryFileNames, derivationsFileNames);
        lemmasMerger.SaveToFile(lemmasMergerIndexFileName);
        return 0;
    }

    if (runMode == Words) {
        NLemmasMerger::TLemmasMerger lemmasMerger;
        lemmasMerger.LoadFromFile(lemmasMergerIndexFileName);

        TString text;
        while (Cin.ReadLine(text)) {
            Cout << JoinSeq(" ", lemmasMerger.ReadText(UTF8ToWide(text), wordsCountLimit)) << Endl;
        }
        return 0;
    }

    if (runMode == Hashes) {
        NLemmasMerger::TLemmasMerger lemmasMerger;
        lemmasMerger.LoadFromFile(lemmasMergerIndexFileName);

        TString text;
        while (Cin.ReadLine(text)) {
            if (textFieldNumber == (size_t) -1) {
                TVector<ui64> unigramHashes = lemmasMerger.ReadHashes(UTF8ToWide(text), wordsCountLimit);
                Cout << JoinSeq(" ", unigramHashes) << "\n";
                continue;
            }

            TStringBuf textBuf(text);
            for (size_t fieldIdx = 0; textBuf; ++fieldIdx) {
                TStringBuf current = textBuf.NextTok('\t');

                if (fieldIdx) {
                    Cout << '\t';
                }

                if (fieldIdx == textFieldNumber) {
                    TVector<ui64> unigramHashes = lemmasMerger.ReadHashes(UTF8ToWide(current), wordsCountLimit);
                    Cout << JoinSeq(" ", unigramHashes);
                } else {
                    Cout << current;
                }
            }

            Cout << "\n";
        }
        return 0;
    }

    Y_ASSERT(0);
    return 1;
}
