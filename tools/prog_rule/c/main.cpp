#include <tools/prog_rule/prog_rule_trie.h>

#include <library/cpp/getopt/last_getopt.h>

#include <util/charset/wide.h>
#include <util/string/vector.h>
#include <util/string/split.h>

namespace NProgRulePrivate {

class TAlphabetBuilder {
private:
    TAlphabet::TSymbol NextSymbol;
    TAlphabet& A;
public:
    TAlphabetBuilder(TAlphabet& a)
        : NextSymbol(0)
        , A(a) {
    }
    TAlphabet::TSymbol Add(const TUtf16String& s) {
        TAlphabet::TMapType::iterator it = A.Map.find(s);
        if (it == A.Map.end()) {
            it = A.Map.insert(std::make_pair(s, NextSymbol++)).first;
        }
        return it->second;
    }
};

typedef TCompactTrieBuilder<TAlphabet::TSymbol, ui32> TTrieBuilder;

}

int main(int argc, char* argv[]) {
    NLastGetopt::TOpts opts;
    opts.AddLongOption('o', "output", "output files base name").RequiredArgument();
    opts.AddHelpOption();
    NLastGetopt::TOptsParseResult optsResult(&opts, argc, argv);
    if (!optsResult.Has("output")) {
        Cerr << "No output files base name specified" << Endl;
        opts.PrintUsage(optsResult.GetProgramName());
        return 1;
    }
    static TUtf16String space(u" ");
    using namespace NProgRulePrivate;
    TString baseFileName = optsResult.Get("output");
    try {
        TTrieBuilder tb;
        TAlphabet ab;
        TAlphabetBuilder abb(ab);
        TString line;
        while (Cin.ReadLine(line)) {
            TUtf16String wline(UTF8ToWide(line));
            TVector<TUtf16String> words;
            StringSplitter(wline).SplitBySet(space.data()).SkipEmpty().Collect(&words);
            TTrie::TKey key;
            for (size_t i = 0; i < words.size(); ++i) {
                TAlphabet::TSymbol s = abb.Add(words[i]);
                key.push_back(s);
            }
            tb.Add(key, 0);
        }
        TOFStream abcStream(baseFileName + ".abc");
        ab.Save(abcStream);
        TOFStream trieStream(baseFileName + ".trie");
        tb.Save(trieStream);
    } catch (yexception& e) {
        Cerr << e.what() << Endl;
        return -1;
    }
    return 0;
}
