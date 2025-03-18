#include <tools/prog_rule/prog_rule_trie.h>

#include <library/cpp/getopt/last_getopt.h>

#include <util/charset/wide.h>
#include <util/string/vector.h>
#include <util/string/vector.h>
#include <util/string/split.h>

int main(int argc, char* argv[]) {
    NLastGetopt::TOpts opts;
    opts.AddLongOption('f', "input", "input files base name").RequiredArgument();
    opts.AddHelpOption();
    NLastGetopt::TOptsParseResult optsResult(&opts, argc, argv);
    if (!optsResult.Has("input")) {
        Cerr << "No input files base name specified" << Endl;
        opts.PrintUsage(optsResult.GetProgramName());
        return 1;
    }
    static TUtf16String space(u" ");
    using namespace NProgRulePrivate;
    TString baseFileName = optsResult.Get("input");
    try {
        TIFStream in(baseFileName + ".abc");
        TAlphabet ab;
        ab.Load(in);
        TBlob trieBlob(TBlob::FromFile(baseFileName + ".trie"));
        TTrie trie(trieBlob);

        TString line;
        while (Cin.ReadLine(line)) {
            TUtf16String wline(UTF8ToWide(line));
            TVector<TUtf16String> words;
            StringSplitter(wline).SplitBySet(space.data()).SkipEmpty().Collect(&words);
            TTrie::TKey key;
            bool foundAllWords = true;
            for (size_t i = 0; i < words.size(); ++i) {
                if (!ab.Has(words[i])) {
                    foundAllWords = false;
                    break;
                }
                key.push_back(ab.Get(words[i]));
            }
            char foundAllWordsC = foundAllWords ? '+' : '-';
            char foundKeyC = foundAllWords && trie.Find(key) ? '+' : '-';
            Cout << foundAllWordsC << "\t" << foundKeyC << Endl;
        }
    } catch (yexception& e) {
        Cerr << e.what() << Endl;
        return -1;
    }
    return 0;
}
