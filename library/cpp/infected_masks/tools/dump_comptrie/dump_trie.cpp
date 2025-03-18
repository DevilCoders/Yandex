#include <library/cpp/colorizer/colors.h>
#include <library/cpp/getopt/last_getopt.h>
#include <library/cpp/infected_masks/masks_comptrie.h>

struct ProgramOptions {
    TVector<TString> TrieFiles;
    bool MakeDifference = false;
    bool OnlyFirstDifference = false;
};

const NColorizer::TColors& colors = NColorizer::StdOut();

void ShowChangedRow(const TCategMaskComptrie::TTrie::TConstIterator& iterator, bool isRemoved) {
    if (isRemoved) {
        Cout << colors.RedColor() << "-";
    } else {
        Cout << colors.GreenColor() << "+";
    }

    Cout << iterator.GetKey() << "\t" << iterator.GetValue() << colors.OldColor() << Endl;
}

void ShowRemovedRow(const TCategMaskComptrie::TTrie::TConstIterator& iterator) {
    ShowChangedRow(iterator, true);
}

void ShowAddedRow(const TCategMaskComptrie::TTrie::TConstIterator& iterator) {
    ShowChangedRow(iterator, false);
}

void ShowDifference(const ProgramOptions& options) {
    TCategMaskComptrie::TTrie oldTrie(TBlob::FromFile(options.TrieFiles[0]));
    TCategMaskComptrie::TTrie newTrie(TBlob::FromFile(options.TrieFiles[1]));

    auto oldIter = oldTrie.begin();
    const auto oldEnd = oldTrie.end();

    auto newIter = newTrie.begin();
    const auto newEnd = newTrie.end();

    bool isRemovedFound = false;
    bool isAddedFound = false;

    while (oldIter != oldEnd && newIter != newEnd) {
        int keyComparison = oldIter.GetKey().compare(newIter.GetKey());

        if (keyComparison == 0) {
            ++oldIter;
            ++newIter;
        } else if (keyComparison < 0) {
            if (!options.OnlyFirstDifference || !isRemovedFound) {
                ShowRemovedRow(oldIter);
            }

            ++oldIter;
            isRemovedFound = true;
        } else {
            if (!options.OnlyFirstDifference || !isAddedFound) {
                ShowAddedRow(newIter);
            }

            ++newIter;
            isAddedFound = true;
        }

        if (options.OnlyFirstDifference && isAddedFound && isRemovedFound) {
            return;
        }
    }

    while (oldIter != oldEnd && (!options.OnlyFirstDifference || !isRemovedFound)) {
        ShowRemovedRow(oldIter);
        ++oldIter;

        isRemovedFound = true;
    }

    while (newIter != newEnd && (!options.OnlyFirstDifference || !isAddedFound)) {
        ShowAddedRow(newIter);
        ++newIter;

        isAddedFound = true;
    }
}

void DumpTrie(const ProgramOptions& options) {
    for (const TString& trieFile : options.TrieFiles) {
        if (options.TrieFiles.size() > 1) {
            Cout << "[" << trieFile << "]" << Endl;
        }

        TCategMaskComptrie::TTrie trie(TBlob::FromFile(trieFile));

        for (const auto& keyValue : trie) {
            Cout << keyValue.first << "\t" << keyValue.second << Endl;
        }
    }
}

int main(int argc, const char* argv[]) {
    ProgramOptions programOptions;
    NLastGetopt::TOpts availableOptions;

    availableOptions.AddHelpOption();

    availableOptions.AddLongOption('d', "diff", "Show difference between two tries")
        .NoArgument()
        .SetFlag(&programOptions.MakeDifference);

    availableOptions.AddLongOption('o', "first-only", "Show only first difference, requires --diff")
        .NoArgument()
        .SetFlag(&programOptions.OnlyFirstDifference);

    availableOptions.SetFreeArgDefaultTitle("TRIE", "Trie file");
    availableOptions.SetFreeArgsMin(1);

    NLastGetopt::TOptsParseResult optsResult(&availableOptions, argc, argv);

    programOptions.TrieFiles = optsResult.GetFreeArgs();

    if (programOptions.OnlyFirstDifference && !programOptions.MakeDifference) {
        Cout << "Option --first-only can be used only with --diff" << Endl;
        return EXIT_FAILURE;
    }

    if (programOptions.MakeDifference && programOptions.TrieFiles.size() != 2) {
        Cout << "Option --diff requires exactly two tries — old and new" << Endl;
        return EXIT_FAILURE;
    }

    if (programOptions.MakeDifference) {
        ShowDifference(programOptions);
    } else {
        DumpTrie(programOptions);
    }
}
