#include "options.h"

#include <library/cpp/charset/codepage.h>

TRunOpts::TRunOpts(int argc, char* argv[]) {
    InitOptParser();
    Parse(argc, argv);
}


void TRunOpts::Parse(int argc, char* argv[]) {
    NLastGetopt::TOptsParseResult optsResult(&Options, argc, argv);
    try {
        if (optsResult.Has("facts")) {
            FactsPath = optsResult.Get("facts");
        }

        if (optsResult.Has("gzt-file")) {
            GztPath = optsResult.Get("gzt-file");
        }

        if (optsResult.Has("input-end-sign")) {
            InputEndSign = optsResult.Get("input-end-sign");
        }
        if (optsResult.Has("output-end-sign")) {
            OutputEndSign = optsResult.Get("output-end-sign");
        }

        ExtractorOpts.OnlyWithNumbers = optsResult.Has("only-with-numbers");
        ExtractorOpts.OnlyWithCity = optsResult.Has("only-with-city");

        ExtractorOpts.CombineWithNextInLine = optsResult.Has("combine-with-next-in-line");
        ExtractorOpts.CombineWithNext = optsResult.Has("combine-with-next");
        ExtractorOpts.CombineWithTheOnlyCity = optsResult.Has("combine-the-only-city");
        ExtractorOpts.CombineWithHeader = optsResult.Has("combine-with-header");

        ExtractorOpts.DebugFacts = optsResult.Has("debug-facts");
        ExtractorOpts.RemoveSmaller = optsResult.Has("remove-smaller");

        ExtractorOpts.DoAutoPrintResult = !optsResult.Has("without-auto-dump-result");

        ThreadsCount = FromString<size_t>(optsResult.Get("threads"));

        if (optsResult.Has("max-facts-to-combine")) {
            try {
                ExtractorOpts.MaxFactNumber = optsResult.Get<size_t>("max-facts-to-combine");
            } catch (const yexception& e) {
                throw NLastGetopt::TUsageException() << "Bad 'max-facts-to-combine' param value: " << e.what();
            }
        }

        try {
            ExtractorOpts.Lang = NLanguageMasks::CreateFromList(optsResult.Get("lang"));
        } catch (const yexception& e) {
            throw NLastGetopt::TUsageException() << "Bad 'lang' param value: " << e.what();
        }

        ExtractorOpts.Encoding = ::CharsetByName(optsResult.Get("encoding"));
        if (CODES_UNSUPPORTED == ExtractorOpts.Encoding) {
            throw NLastGetopt::TUsageException() << "Unsupported encoding: " << optsResult.Get("encoding");
        }

    } catch (const yexception&) {
        optsResult.HandleError();
        exit(1);
    }
}

void TRunOpts::InitOptParser() {
    Options.AddLongOption('f', "facts", "fact definition file name").RequiredArgument().Required();
    Options.AddLongOption('g', "gzt-file", "gazetteer dictionary file name").RequiredArgument();
    Options.AddLongOption('i', "input-end-sign", "line to wait for").RequiredArgument();
    Options.AddLongOption('o', "output-end-sign", "line to print after output").RequiredArgument();
    Options.AddLongOption('n', "only-with-numbers", "print only addresses with street and numbers").NoArgument();
    Options.AddLongOption('c', "only-with-city", "print only addresses with city").NoArgument();
    Options.AddLongOption('m', "max-facts-to-combine", "maximum number of facts to keep in memory for combining").RequiredArgument().DefaultValue("200");

    Options.AddLongOption("combine-with-next-in-line", "Combine geo to next in line").NoArgument();
    Options.AddLongOption("combine-with-next", "Combine geo to next").NoArgument();
    Options.AddLongOption("combine-the-only-city", "Combine the only geo can be agreed with all addresses").NoArgument();
    Options.AddLongOption("combine-with-header", "Combine addresses with city in list header").NoArgument();

    Options.AddLongOption("without-auto-dump-result", "Test mode without dump results").NoArgument();

    Options.AddLongOption("remove-smaller", "Remove strongly smaller facts").NoArgument();
    Options.AddLongOption('d', "debug-facts", "Print debug information for every fact").NoArgument();
    Options.AddLongOption('l', "lang", "comma-separated list of input file languages").RequiredArgument().DefaultValue("rus");
    Options.AddLongOption('e', "encoding", "input encoding").RequiredArgument().DefaultValue("utf-8");

    Options.AddLongOption('t', "threads", "Threads count (for multithreading testing)").RequiredArgument().DefaultValue("1");

    Options.AddHelpOption();
}
