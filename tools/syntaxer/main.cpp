#include "printer.h"

#include <dict/light_syntax/simple/simple.h>

#include <kernel/remorph/tokenizer/multitoken_split.h>

#include <library/cpp/getopt/last_getopt.h>

#include <util/charset/wide.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <util/generic/yexception.h>
#include <util/stream/input.h>
#include <util/stream/output.h>
#include <util/string/vector.h>

using namespace NSyntaxer;

namespace {

inline void Process(const NLightSyntax::TSimpleLightSyntaxParser& parser, const TPrinter& printer,
                    const TUtf16String& text, NToken::EMultitokenSplit multitokenSplit) {
    TVector<NLightSyntax::TSimplePhrase> phrases = parser.Parse(text, multitokenSplit);
    printer.Print(phrases, text);
}

}

int main(int argc, char* argv[]) {
    try {
        NLastGetopt::TOpts opts;
        opts.AddLongOption('l', "langs", "languages").RequiredArgument();
        opts.AddLongOption('m', "multitoken-split", "multitoken splitting mode ('minimal', 'smart', 'all'), default is 'minimal'").RequiredArgument();
        opts.AddHelpOption();
        opts.SetFreeArgsMin(0);
        opts.SetFreeArgTitle(0, "input", "word from input phrase (skip for batch mode)");
        NLastGetopt::TOptsParseResult o(&opts, argc, argv);
        TVector<TString> args = o.GetFreeArgs();

        bool optionsError = false;

        TString langs = o.GetOrElse("langs", "");

        NToken::EMultitokenSplit multitokenSplit = NToken::MS_MINIMAL;
        if (o.Has("multitoken-split")) {
            TString ms = o.Get("multitoken-split");
            if (ms == "minimal") {
                multitokenSplit = NToken::MS_MINIMAL;
            } else if (ms == "smart") {
                multitokenSplit = NToken::MS_SMART;
            } else if (ms == "all") {
                multitokenSplit = NToken::MS_ALL;
            } else {
                Cerr << "Incorrect multitoken split mode: " << ms.Quote() << Endl;
                optionsError = true;
            }
        }

        if (optionsError) {
            opts.PrintUsage(o.GetProgramName());
            return 1;
        }

        NLightSyntax::TSimpleLightSyntaxParser parser(langs);
        TPrinter printer(Cout);

        if (args.empty()) {
            TString line;
            TUtf16String text;

            size_t lineCount = 0;
            while (Cin.ReadLine(line)) {
                try {
                    ++lineCount;
                    if (line.empty()) {
                        continue;
                    }
                    Process(parser, printer, ::UTF8ToWide<true>(line), multitokenSplit);
                } catch (const yexception& error) {
                    Cerr << "WARN: " << error.what() << ", line " << lineCount << " (" << line << ")" << Endl;
                }
            }
        } else {
            TUtf16String text = ::UTF8ToWide<true>(::JoinStrings(args, " "));
            Process(parser, printer, text, multitokenSplit);
        }
    } catch (yexception& error) {
        Cerr << "ERROR: " << error.what() << Endl;
        return 2;
    }
    return 0;
}
