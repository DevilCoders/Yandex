#include <kernel/hosts/owner/owner.h>
#include <library/cpp/containers/comptrie/comptrie_builder.h>
#include <library/cpp/getopt/last_getopt.h>
#include <library/cpp/uri/other.h>
#include <util/system/datetime.h>

struct ProgramOptions {
    bool Compact = false;
    bool MakeFastLayout = false;
    TString SourceFileName;
    TString DestinationFileName;
    bool MakeMask = false;
    bool CreateTagFile = false;
    bool EmbedCreationTime = false;
};

TString ConvertUrlToMaskKey(const TOwnerCanonizer& ownerCanonizer, const TStringBuf& url) {
    TString invertedDomain(url);
    InvertDomain(invertedDomain);

    return ownerCanonizer.GetUrlOwner(url) + TString("\t") + invertedDomain;
}

void CreateSimpleTrie(const ProgramOptions& options) {
    TString creationTime = ToString(Seconds());

    TCompactTrieBuilder<char, TStringBuf> builder;
    TFileInput in(options.SourceFileName);

    TVector<TString> parts;
    TString line;

    TOwnerCanonizer ownerCanonizer;
    ownerCanonizer.LoadTrueOwners();

    while (in.ReadLine(line)) {
        TStringBuf key, value;
        TStringBuf buffer = line;
        buffer.RSplit('\t', key, value);

        if (options.MakeMask) {
            builder.Add(ConvertUrlToMaskKey(ownerCanonizer, key), value);
        } else {
            builder.Add(key, value);
        }
    }

    if (options.EmbedCreationTime) {
        builder.Add("@creationTime", creationTime);
    }

    auto out = TFileOutput{options.DestinationFileName};

    if (options.Compact) {
        if (options.MakeFastLayout) {
            CompactTrieMinimizeAndMakeFastLayout(out, builder, true);
        } else {
            CompactTrieMinimize(out, builder, true);
        }
    } else {
        if (options.MakeFastLayout) {
            CompactTrieMakeFastLayout(out, builder, true);
        } else {
            builder.Save(out);
        }
    }

    if (options.CreateTagFile) {
        TString tagFile = options.DestinationFileName + ".tag";
        TFileOutput tagOut(tagFile);
        tagOut << "creationTime:" << creationTime << Endl;
    }
}

int main(int argc, const char* argv[]) {
    ProgramOptions programOptions;
    NLastGetopt::TOpts availableOptions;

    availableOptions.AddLongOption('c', "compact", "Compact trie")
        .OptionalArgument()
        .StoreResult(&programOptions.Compact, true)
        .DefaultValue(true);

    availableOptions.AddLongOption('f', "fast", "Make fast trie layout")
        .OptionalArgument()
        .StoreResult(&programOptions.MakeFastLayout, true)
        .DefaultValue(false);

    availableOptions.AddLongOption('d', "destination", "Save trie to file")
        .Required()
        .RequiredArgument()
        .StoreResult(&programOptions.DestinationFileName);

    availableOptions.AddLongOption('s', "source", "Load key-value pairs from file")
        .Required()
        .RequiredArgument()
        .StoreResult(&programOptions.SourceFileName);

    availableOptions.AddLongOption('m', "mask", "Convert each key to mask")
        .NoArgument()
        .SetFlag(&programOptions.MakeMask);

    availableOptions.AddLongOption('t', "create-tag", "Create .tag file")
        .NoArgument()
        .SetFlag(&programOptions.CreateTagFile);

    availableOptions.AddLongOption('e', "embed-creation-time", "Embed creation time into trie")
        .NoArgument()
        .SetFlag(&programOptions.EmbedCreationTime);

    availableOptions.SetFreeArgsMax(0);
    availableOptions.AddHelpOption('h');

    NLastGetopt::TOptsParseResult optsResult(&availableOptions, argc, argv);

    CreateSimpleTrie(programOptions);
}
