#include <kernel/inflectorlib/pluralize/pluralize.h>

#include <kernel/lemmer/dictlib/grammar_index.h>
#include <library/cpp/streams/factory/factory.h>
#include <library/cpp/getopt/last_getopt.h>

#include <util/charset/wide.h>
#include <util/stream/input.h>
#include <util/stream/output.h>
#include <util/string/cast.h>
#include <util/string/strip.h>

int main(int argc, char * argv[]) {
    NLastGetopt::TOpts opts;

    opts.SetTitle("Simple demo tool for pluralization library");

    TString inputPath;
    opts.AddLongOption('i', "input", "file with input phrase, one per line")
        .RequiredArgument("PATH")
        .Optional()
        .DefaultValue("-")
        .StoreResult(&inputPath);

    ui64 number = 1;
    opts.AddLongOption('n', "number", "number for pluralization")
        .RequiredArgument("NUM")
        .Required()
        .StoreResult(&number);

    bool sweep = false;
    opts.AddLongOption('w', "sweep", "run through all numbers from 0 to n")
        .NoArgument()
        .Optional()
        .StoreValue(&sweep, true);

    TString targetCaseStr;
    opts.AddLongOption('c', "case", "target case: nom, dat, gen, acc, ins, abl; '-' for all")
        .RequiredArgument("CASE")
        .Optional()
        .DefaultValue("-")
        .StoreResult(&targetCaseStr);

    bool animated = false;
    opts.AddLongOption("anim", "phrase refers to animated object; currently needed to correctly inflect for case=acc")
        .NoArgument()
        .Optional()
        .StoreValue(&animated, true);

    bool singularize = false;
    opts.AddLongOption('s', "singularize", "singularize instead of pluralizing")
        .NoArgument()
        .Optional()
        .StoreValue(&singularize, true);

    NLastGetopt::TOptsParseResult optionsResult(&opts, argc, argv);
    Y_UNUSED(optionsResult);

    TVector<EGrammar> targetCases;
    if (targetCaseStr == TStringBuf("-")) {
        targetCases = TVector<EGrammar>{gNominative, gDative, gGenitive, gAccusative, gInstrumental, gAblative};
    } else {
        targetCases = TVector<EGrammar>{TGrammarIndex::GetCode(targetCaseStr)};
    }

    THolder<IInputStream> input = OpenInput(inputPath);

    TUtf16String text;
    while (input->ReadLine(text)) {
        Strip(text);
        if (text.empty()) {
            continue;
        }

        if (singularize) {
            Cout << NInfl::Singularize(text, number, LANG_RUS) << Endl;
        } else {
            ui64 startFrom = sweep ? 0 : number;
            for (ui64 curNumber = startFrom; curNumber <= number; ++curNumber) {
                for (EGrammar targetCase : targetCases) {
                    Cout << TGrammarIndex::GetUtf8Name(targetCase)
                        << "\t" << curNumber
                        << " " << NInfl::Pluralize(text, curNumber, targetCase, LANG_RUS, animated) << Endl;
                }
            }
            Cout << Endl;
        }
    }
}

