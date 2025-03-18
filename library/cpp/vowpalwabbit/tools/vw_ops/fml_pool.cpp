#include <library/cpp/getopt/last_getopt.h>

#include <util/stream/file.h>
#include <util/string/cast.h>
#include <util/string/subst.h>

int ConvertFmlPool2VwPool(int argc, const char** argv) {
    TString fmlPool;
    TString vwPool;
    float threshold = 0;
    bool useWeights = false;
    TString zeroFactorsStr;

    auto opts = NLastGetopt::TOpts::Default();
    opts.SetFreeArgsNum(0);

    opts.AddLongOption('f', "fmlpool", "[in] Pool in FML format").Required().RequiredArgument("fmlpool").StoreResult(&fmlPool);
    opts.AddLongOption('v', "vwpool", "[out] Pool in VW format").Required().RequiredArgument("vwpool").StoreResult(&vwPool);
    opts.AddLongOption('t', "threshold", "Target value threshold. Values less that threshold will be mapped to -1, above will be mapped to 1")
            .Required()
            .RequiredArgument("threshold")
            .StoreResult(&threshold);
    opts.AddLongOption('w', "use-weights", "Use weights from 4-th column of the pool.")
            .Optional()
            .NoArgument()
            .StoreValue(&useWeights, true);
    opts.AddLongOption('z', "zero-factors", "Comma (,) or colon (:) separated list of factors or ranges of factors to exclude from vw pool (for example: '0:2:4:10-16').")
            .Optional()
            .RequiredArgument("zero-factors")
            .StoreResult(&zeroFactorsStr);

    NLastGetopt::TOptsParseResult parsedOpts(&opts, argc, argv);
    TFileInput in(fmlPool);
    TFileOutput out(vwPool);
    TString line;
    THashSet<size_t> zeroFactors;

    if (zeroFactorsStr) {
        for (TStringBuf factor : StringSplitter(zeroFactorsStr).SplitBySet(",:")) {
            if (factor.Contains('-')) {
                size_t begin = 0;
                size_t end = 0;
                StringSplitter(factor).Split('-').CollectInto(&begin, &end);
                Y_VERIFY(begin < end);
                for (size_t i = begin; i <= end; ++i) {
                    zeroFactors.insert(i);
                }
            } else {
                zeroFactors.insert(FromString<size_t>(factor));
            }
        }
    }

    while (in.ReadLine(line)) {
        TVector<TStringBuf> parts = StringSplitter(line).Split('\t');
        if (parts.size() < 5) {
            Cerr << "Invalid FML pool format: " << line << Endl;
            return -1;
        }

        const auto target = FromString<float>(parts[1]);
        if (target > threshold) {
            out.Write("1 ");
        } else {
            out.Write("-1 ");
        }

        if (useWeights) {
            out.Write(parts[3]);
        } else {
            out.Write("1");
        }

        out.Write(" '");

        TString label = ToString(parts[2]);
        SubstGlobal(label, '|', '_');
        SubstGlobal(label, ' ', '_');
        out.Write(label);
        out.Write(" |");

        for (size_t i = 4; i < parts.size(); ++i) {
            const size_t factor = i - 4;
            if (parts[i] == TStringBuf("nan") ||
                parts[i].find_first_not_of(TStringBuf("0.")) == TStringBuf::npos ||
                zeroFactors.contains(factor))
            {
                continue;
            }
            out.Write(' ');
            out.Write(ToString(factor));
            out.Write(':');
            out.Write(parts[i]);
        }
        out.Write('\n');
    }

    return 0;
}
