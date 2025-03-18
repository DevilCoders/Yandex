#include "printer.h"

#include <util/generic/string.h>
#include <util/system/defaults.h>

namespace NSyntaxer {

namespace {

inline void PrintPhrases(IOutputStream& output, const TVector<NLightSyntax::TSimplePhrase>& phrases,
                        const TWtringBuf& text, i64 head = -1, size_t depth = 0) {
    i64 n = 0;
    for (TVector<NLightSyntax::TSimplePhrase>::const_iterator phraseIt = phrases.begin();
         phraseIt != phrases.end(); ++phraseIt) {
        const NLightSyntax::TSimplePhrase& phrase = *phraseIt;
        output << TString(depth, ' ') << "[" << phrase.Name << "]";
        if (phrase.Weight) {
            output << " (" << phrase.Weight << ")";
        }
        output << " " << text.substr(phrase.Begin, phrase.End - phrase.Begin);
        if (head == n) {
            output << " *";
        }
        output << Endl;
        PrintPhrases(output, phrase.Children, text, phrase.Head, depth + 1);
        ++n;
    }
}

}

TPrinter::TPrinter(IOutputStream& output)
    : Output(output)
{
}

void TPrinter::Print(const TVector<NLightSyntax::TSimplePhrase>& phrases, const TWtringBuf& text) const {
    PrintPhrases(Output, phrases, text);
}

} // NSyntaxer
