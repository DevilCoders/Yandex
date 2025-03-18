#include "printer.h"

#include <kernel/inflectorlib/phrase/simple/simple.h>

#include <library/cpp/getopt/last_getopt.h>

#include <util/charset/wide.h>
#include <util/generic/strbuf.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <util/generic/yexception.h>
#include <util/stream/input.h>
#include <util/stream/output.h>
#include <util/string/vector.h>

using namespace NInflector;

namespace {

inline void Process(const NInfl::TSimpleInflector& inflector, const TPrinter& printer, const TUtf16String& text,
                    const TString& grams) {
    NInfl::TSimpleResultInfo resultInfo;
    TUtf16String inflected = inflector.Inflect(text, grams, &resultInfo);
    printer.Print(inflected, resultInfo);
}

inline void ParsePhraseOptions(const TStringBuf& str, TString& grams) {
    TStringBuf opts = str;
    TStringBuf opt;
    while (opts.NextTok(';', opt)) {
        if (opt.empty()) {
            continue;
        }
        size_t equalsPos = opt.find('=');
        bool supported = true;
        if ((equalsPos != TStringBuf::npos) && (equalsPos != 0) && (equalsPos != opt.size() - 1)) {
            TStringBuf name = opt.substr(0, equalsPos);
            TStringBuf value = opt.substr(equalsPos + 1);
            if ((name == "g") || (name == "grams")) {
                grams = value;
            } else {
                supported = false;
            }
        } else {
            supported = false;
        }
        if (!supported) {
            throw yexception() << "Unsupported per-phrase option: \"" << opt << "\"";
        }
    }
}

}

int main(int argc, char* argv[]) {
    try {
        NLastGetopt::TOpts opts;
        opts.AddLongOption('g', "grams", "grammemes").RequiredArgument();
        opts.AddLongOption('l', "langs", "languages").RequiredArgument();
        opts.AddLongOption('e', "extended", "extended input format (opt1=value1;opt2=value2 <tab> text").NoArgument();
        opts.AddLongOption('v', "verbose", "verbose mode").NoArgument();
        opts.AddHelpOption();
        opts.SetFreeArgsMin(0);
        opts.SetFreeArgTitle(0, "input", "word from input phrase (skip for batch mode)");
        NLastGetopt::TOptsParseResult o(&opts, argc, argv);
        TVector<TString> args = o.GetFreeArgs();

        bool optionsError = false;

        if (o.Has("extended") && !args.empty()) {
            Cerr << "Options --extended and non-batch mode are not compatible." << Endl;
            optionsError = true;

        }

        TString grams = o.GetOrElse("grams", "");
        TString langs = o.GetOrElse("langs", "");

        bool extended = o.Has("extended");
        bool verbose = o.Has("verbose");

        if (optionsError) {
            opts.PrintUsage(o.GetProgramName());
            return 1;
        }

        NInfl::TSimpleInflector inflector(langs);
        TPrinter printer(Cout, verbose);

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
                    if (extended) {
                        TString localGrams = grams;
                        size_t tabPos = line.find('\t');
                        if (tabPos != TString::npos) {
                            ParsePhraseOptions(line.substr(0, tabPos), localGrams);
                            line.erase(0, tabPos + 1);
                        }
                        Process(inflector, printer, ::UTF8ToWide<true>(line), localGrams);
                    } else {
                        Process(inflector, printer, ::UTF8ToWide<true>(line), grams);
                    }
                } catch (const yexception& error) {
                    Cerr << "WARN: " << error.what() << ", line " << lineCount << " (" << line << ")" << Endl;
                }
            }
        } else {
            TUtf16String text = ::UTF8ToWide<true>(::JoinStrings(args, " "));
            Process(inflector, printer, text, grams);
        }
    } catch (yexception& error) {
        Cerr << "ERROR: " << error.what() << Endl;
        return 2;
    }
    return 0;
}
