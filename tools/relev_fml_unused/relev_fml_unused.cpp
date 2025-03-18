#include "factors_usage_tester.h"

#include <library/cpp/getopt/last_getopt.h>

#include <util/stream/output.h>

int Main(int argc, char* argv[]) {
    NLastGetopt::TOpts opts;

    opts.SetTitle(
        "relev_fml_unused: a tool to monitor unused factors\n"
        "Documentation: https://nda.ya.ru/3QTVna"
    );

    bool testUnusedFactors = false;
    opts.AddLongOption('u', "unused",
            "Check usage of TG_DEPRECATED (but not TG_REMOVED) factors in ranking models.")
        .Optional()
        .NoArgument()
        .StoreValue(&testUnusedFactors, true);

    bool testRemovedFactors = false;
    opts.AddLongOption('r', "removed",
        "Check usage of TG_REMOVED factors in ranking models.")
        .Optional()
        .NoArgument()
        .StoreValue(&testRemovedFactors, true);

    bool showUnusedFactorsInfo = false;
    opts.AddLongOption('i', "show-unused",
        "Show where TG_DEPRECATED factors are still used.")
        .Optional()
        .NoArgument()
        .StoreValue(&showUnusedFactorsInfo, true);

    bool showUnusedFactorsGroupByID = false;
    opts.AddLongOption("unused-id",
        "Works with show-unused option, grouping result by factor IDs.")
        .Optional()
        .NoArgument()
        .StoreValue(&showUnusedFactorsGroupByID, true);

    bool testRearrangeFactors = false;
    opts.AddLongOption("rearr",
        "Check rearranging factors that marked as TG_UNUSED/TG_DEPRECATED/TG_REMOVED.")
        .Optional()
        .NoArgument()
        .StoreValue(&testRearrangeFactors, true);

    bool testUnusedRearrangeFactors = false;
    opts.AddLongOption("rearr-unused",
        "Check rearranging factors that marked as TG_UNUSED/TG_DEPRECATED/TG_REMOVED and not used in any formula.")
        .Optional()
        .NoArgument()
        .StoreValue(&testUnusedRearrangeFactors, true);

    bool testUnimplementedFactors = false;
    opts.AddLongOption("unimplemented",
        "Check usage of TG_UNIMPLEMENTED factors in ranking models.")
        .Optional()
        .NoArgument()
        .StoreValue(&testUnimplementedFactors, true);

    bool useDefaultFormulas = true;
    opts.AddLongOption('e', "no-default",
        "Don't use default list of files with ranking models.")
        .Optional()
        .NoArgument()
        .StoreValue(&useDefaultFormulas, false);

    TString formulasList;
    opts.AddLongOption('l', "list",
        "File with custom list of .fml/.factors files to check.")
        .Optional()
        .RequiredArgument("FILE")
        .StoreResult(&formulasList);

    TVector<TString> modelsArchives;
    opts.AddLongOption('a', "archive",
        "Additional archive with .info formulas (models.archive). There can be more than one -a options.")
        .Optional()
        .RequiredArgument("FILE")
        .AppendTo(&modelsArchives);

    bool justPrintDefaultFormulas = false;
    opts.AddLongOption('L', "formulas-list",
        "Just print the default formulas used as the sources.")
        .Optional()
        .NoArgument()
        .StoreValue(&justPrintDefaultFormulas, true);

    TString sourcesPath = Y_STRINGIZE(RFU_ARCADIA_ROOT);
    opts.AddLongOption('A', "arc",
        "Path to arcadia source tree root.")
        .Optional()
        .RequiredArgument("DIR")
        .StoreResult(&sourcesPath)
        .DefaultValue(Y_STRINGIZE(RFU_ARCADIA_ROOT));

    int testSpecificFactor = -1;
    opts.AddLongOption('o', "factor",
        "Check that all the factors not marked as TG_DEPRECATED/TG_REMOVED are used in some formula.")
        .Optional()
        .RequiredArgument("FACTOR_ID")
        .StoreResult(&testSpecificFactor);

    int showFactorUsage = -1;
    opts.AddLongOption('f', "show-usage",
        "Show all formulas where specified factor is used.")
        .Optional()
        .RequiredArgument("FACTOR_ID")
        .StoreResult(&showFactorUsage);

    opts.AddLongOption("formulas",
        "Specify file with formulas for checking")
        .DefaultValue("tools/relev_fml_unused/web_formulas");

    TString factorsType;
    opts.AddLongOption('t', "type",
        "Factors type (blender|web)")
        .Optional()
        .DefaultValue("web")
        .StoreResult(&factorsType);

    bool showIgnoreFactors = false;
    opts.AddLongOption("ignore",
        "Show factors that shouldn't be used in new formulas (for -I matrixnet option)")
        .Optional()
        .NoArgument()
        .StoreValue(&showIgnoreFactors, true);

    opts.SetFreeArgTitle(0, "<f1>", "Additional 1st .fml/.factors file to check");
    opts.SetFreeArgTitle(1, "...",  "...");
    opts.SetFreeArgTitle(2, "<fn>", "Additional n-th .fml/.factors file to check");

    opts.AddHelpOption();

    const NLastGetopt::TOptsParseResult optsres(&opts, argc, argv);

    const TString allFormulas = optsres.Get<TString>("formulas");

    Y_ENSURE(sourcesPath.length() > 0, "Can't detect arcadia source directory. ");

    if (sourcesPath.back() != '/') {
        sourcesPath += '/';
    }

    if (formulasList.empty()) {
        formulasList = sourcesPath + allFormulas;
    }

    THolder<IFactorsSet> factorsSet;
    if (factorsType == "web")
        factorsSet.Reset(new TWebFactorsSet(factorsType));
    else if (factorsType == "blender")
        factorsSet.Reset(new TBlenderFactorsSet(factorsType));
    else
        ythrow yexception() << "Incorrect factors type" << Endl;

    TFactorsUsageTester factorsUsageTester(factorsSet.Get());

    if (showIgnoreFactors) {
        Cout << factorsUsageTester.ShowIgnoreFactors() << Endl;
        return 0;
    }

    // Load default formulas
    if (useDefaultFormulas || justPrintDefaultFormulas) {
        factorsUsageTester.LoadDefaultFormulas(formulasList, sourcesPath, justPrintDefaultFormulas);
    }

    // Load custom formulas
    TVector<TString> fmls = optsres.GetFreeArgs();
    if (fmls.size() > 0) {
        for (size_t i = 0; i < fmls.size(); ++i) {
            factorsUsageTester.AddFormula(fmls[i].data());
        }
    } else {
        Y_ENSURE(useDefaultFormulas, "You must specify at least one .fml/.factors "
                "file or remove the '-e' option. ");
    }

    // Load formulas from the .archive files
    for (const auto& archive: modelsArchives) {
        factorsUsageTester.AddFormulasFromArchive(archive, justPrintDefaultFormulas);
    }

    if (justPrintDefaultFormulas) {
        return 0;
    }

    if (!testUnusedFactors && testSpecificFactor < 0 && !testRemovedFactors
            && !showUnusedFactorsInfo && showFactorUsage < 0
            && !testRearrangeFactors && !testUnusedRearrangeFactors
            && !testUnimplementedFactors) {
        // Test TG_DEPRECATED factors by default
        testUnusedFactors = true;
    }

    TString result;
    if (testUnusedFactors) {
        result += factorsUsageTester.TestUnusedFactors();
    }

    if (testSpecificFactor >= 0) {
        result += factorsUsageTester.TestUnusedButNotMarkedFactors(testSpecificFactor);
    }

    if (testRemovedFactors) {
        result += factorsUsageTester.TestRemovedFactors();
    }

    if (testRearrangeFactors) {
        result += factorsUsageTester.TestRearrangeFactors();
    }

    if (testUnusedRearrangeFactors) {
        result += factorsUsageTester.TestUnusedRearrangeFactors();
    }

    if (testUnimplementedFactors) {
        result += factorsUsageTester.TestUnimplementedFactors();
    }

    if (showUnusedFactorsInfo) {
        Cout << factorsUsageTester.GetInfoForUnusedFactors(showUnusedFactorsGroupByID) << Endl;
    }

    if (showFactorUsage >= 0) {
        result += factorsUsageTester.ShowFactorUsage(showFactorUsage);
    }

    if (result.empty())
        return 0;

    Cerr << result << Endl;
    return 1;
}

int main(int argc, char* argv[]) {
    try {
        return Main(argc, argv);
    } catch (const yexception&) {
        Cerr << CurrentExceptionMessage() << Endl;
        return 1;
    }
}
