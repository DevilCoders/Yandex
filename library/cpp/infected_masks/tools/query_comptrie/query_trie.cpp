#include <library/cpp/infected_masks/masks_comptrie.h>
#include <library/cpp/getopt/last_getopt.h>

#include "mode.h"

struct ProgramOptions {
    EMode Mode = EMode::Mask;
    TString TrieFileName;
    bool ReadStdin = false;
};

class ComptrieQuery {
private:
    EMode Mode;
    TCategMaskComptrie Trie;

    TCategMaskComptrie::TResult Values;
    TStringBuf Value;

public:
    explicit ComptrieQuery(const ProgramOptions& programOptions) {
        Mode = programOptions.Mode;
        Trie.Init(TBlob::FromFile(programOptions.TrieFileName));

        Values.reserve(10);
    }

    void QueryArguments(TVector<TString> toFind) {
        for (const auto& key : toFind) {
            Query(key);
        }
    }

    void QueryStdin() {
        TString line;

        while (Cin.ReadLine(line)) {
            Query(line);
        }
    }

private:
    void Query(const TString& key) {
        switch (Mode) {
            case EMode::Mask:
                if (Trie.FindByUrl(key, Values)) {
                    Cout << key << "\t" << ComptrieResultToString() << Endl;
                }
                break;

            case EMode::Exact:
                if (Trie.FindExact(key, Value)) {
                    Cout << key << "\t" << Value << Endl;
                }
                break;

            case EMode::Raw:
                if (Trie.Find(key, Values)) {
                    Cout << key << "\t" << ComptrieResultToString() << Endl;
                }
                break;
        }
    }

    TString ComptrieResultToString() {
        TStringStream stream;

        for (const auto& pair : Values) {
            stream << pair.second << ", ";
        }

        TString str = stream.Str();
        return str.substr(0, str.length() - 2);
    }
};

int main(int argc, char** argv) {
    ProgramOptions programOptions;
    NLastGetopt::TOpts availableOptions;

    availableOptions.AddLongOption('m', "mode", "Search mode: 'mask', 'exact' or 'raw'")
        .RequiredArgument()
        .StoreResult(&programOptions.Mode)
        .DefaultValue(programOptions.Mode);

    availableOptions.AddLongOption('t', "trie", "Trie file")
        .Required()
        .RequiredArgument()
        .StoreResult(&programOptions.TrieFileName);

    availableOptions.AddLongOption('s', "stdin", "Read keys from stdin")
        .NoArgument()
        .StoreResult(&programOptions.ReadStdin, true)
        .DefaultValue(programOptions.ReadStdin);

    availableOptions.AddHelpOption('h');
    availableOptions.SetFreeArgDefaultTitle("KEY", "Key to find in trie");

    NLastGetopt::TOptsParseResult optsResult(&availableOptions, argc, argv);

    auto comptrieQuery = ComptrieQuery{programOptions};

    comptrieQuery.QueryArguments(optsResult.GetFreeArgs());

    if (programOptions.ReadStdin) {
        comptrieQuery.QueryStdin();
    }
}
