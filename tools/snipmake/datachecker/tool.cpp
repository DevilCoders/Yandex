#include <yweb/robot/kiwi_queries/richsnippets/triggers/datachecker/patterns.h>

#include <util/stream/file.h>

int main(int argc, char** argv) {
    if (argc < 2) {
        Cerr << "No input file name!" << Endl;
        return 1;
    }
    TString patterns[] = {
        NRichSnippets::SchemaOrgItemPattern("recipe"),
        NRichSnippets::MicroformatsItemPattern("hrecipe"),
        NRichSnippets::SchemaOrgItemPattern("videoobject"),
    };
    try {
        TFileInput in(argv[1]);
        TString str(in.ReadAll());
        const size_t patternsSize = Y_ARRAY_SIZE(patterns);
        NPire::TScanner scanner(NRichSnippets::ProductOfferImageScanner());
        NRichSnippets::ExtendScanner(scanner, TVector<TString>(patterns, patterns + patternsSize));
        if (!scanner.Empty()) {
            TVector<bool> matches(patternsSize + 1, false);
            NRichSnippets::GetMatches(str, scanner, matches);
            for (size_t i = 0; i < patternsSize + 1; ++i) {
                if (matches[i]) {
                    Cout << i << '\n';
                }
            }
            Cout << Endl;
        } else {
            Cerr << "Scanner is empty" << Endl;
            return 1;
        }
        return 0;
    } catch (const yexception& ex) {
        Cerr << ex.what() << Endl;
        return 1;
    }
}
