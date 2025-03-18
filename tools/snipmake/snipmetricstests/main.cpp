#include <tools/snipmake/snipmetrics/dump.h>
#include <tools/snipmake/snipmetrics/snip2serpiter.h>
#include <tools/snipmake/snipmetrics/snipiter.h>
#include <tools/snipmake/snipmetrics/snipmetrics.h>
#include <tools/snipmake/common/common.h>
#include <util/string/split.h>

namespace NSnippets {

const TString testDataFile = "metricstests_data.txt";

static const TChar DELIM_T[] = {TChar('\t'), 0};
static const TChar DELIM_METRICS[] = {TChar(' '), 0};
static const TChar DELIM_METRIC_VAL[] = {TChar(':'), 0};

int Run(int argc, char** argv)
{
    ++argv; --argc;

    if (argc < 2) {
        Cout << "expected args: arcadia_tests_data porno_words_config" << Endl;
        return 1;
    }
    const TString arcadiaTestsData = argv[0];
    const TString pornoConfig = argv[1];


    TUnbufferedFileInput testDataInput(testDataFile);
    TString currentString;
    THashSet<TUtf16String> seenMetrics;

    while (testDataInput.ReadLine(currentString))
    {
        size_t pos;
        while ((pos = currentString.find("\t\t")) != TString::npos)
        {
            currentString.replace(pos, 2, "\t \t");
        }

        TStringStream in;
        TStringStream out;
        TVector<TUtf16String> inputFields;
        StringSplitter(UTF8ToWide(currentString)).SplitByString(DELIM_T).SkipEmpty().Collect(&inputFields);
        if (inputFields.size() < 8)
        {
            continue;
        }

        // Parse correct metric values
        TVector<TUtf16String> corMetricNameVals;
        StringSplitter(inputFields[7]).SplitByString(DELIM_METRICS).SkipEmpty().Collect(&corMetricNameVals);
        THashMap<TUtf16String, double> correctMetricVals;
        for (TVector<TUtf16String>::const_iterator it = corMetricNameVals.begin(); it != corMetricNameVals.end(); ++it)
        {
            TVector<TUtf16String> metricNameVal;
            StringSplitter(*it).SplitByString(DELIM_METRIC_VAL).SkipEmpty().Collect(&metricNameVal);
            if (metricNameVal.size() < 2)
                continue;
            correctMetricVals[metricNameVal[0]] = FromString<double>(WideToUTF8(metricNameVal[1]));
        }
        Cout << inputFields[0] << " url:" << inputFields[2] << Endl;
        in << "1" << DELIM_T << WideToUTF8(inputFields[0]) << DELIM_T << WideToUTF8(inputFields[1]) << DELIM_T << WideToUTF8(inputFields[2]) << DELIM_T << WideToUTF8(inputFields[3]) <<  DELIM_T << WideToUTF8(inputFields[4])  << DELIM_T << WideToUTF8(inputFields[5]) << DELIM_T << " " << DELIM_T << WideToUTF8(inputFields[6]) << DELIM_T << Endl;


        THolder<ISerpsIterator> iter(new TSnip2SerpIter(new TSnipRawIter(&in)));
        TSnipMetricsDumper<TSimpleSnippetDumper>* calc = new TSnipMetricsDumper<TSimpleSnippetDumper>("Test", arcadiaTestsData, pornoConfig, &out);
        TSnippetsMetricsCalculationApplication app(iter.Get(), calc);

        app.Run();

        TString resString = out.ReadAll();
        TVector<TUtf16String> outFields;
        StringSplitter(UTF8ToWide(resString)).SplitByString(DELIM_T).SkipEmpty().Collect(&outFields);
        if (outFields.size() < 4)
        {
            continue;
        }
        TVector<TUtf16String> metricVals;
        StringSplitter(outFields[3]).SplitByString(DELIM_METRICS).SkipEmpty().Collect(&metricVals);

        seenMetrics.clear();

        for (TVector<TUtf16String>::const_iterator it = metricVals.begin(); it != metricVals.end(); ++it)
        {
            TVector<TUtf16String> metricNameVal;
            StringSplitter(*it).SplitByString(DELIM_METRIC_VAL).SkipEmpty().Collect(&metricNameVal);
            if (metricNameVal.size() < 2)
            {
                continue;
            }
            TUtf16String metricName = metricNameVal[0];
            TUtf16String metricVal = metricNameVal[1];
            if (correctMetricVals.find(metricName) != correctMetricVals.end())
            {
                seenMetrics.insert(metricName);
                if (correctMetricVals[metricName] != FromString<double>(WideToUTF8(metricVal)))
                    Cout << "Error: " << metricName << " - " << metricVal << Endl;
            }

       }
       for (THashMap<TUtf16String,double>::const_iterator it = correctMetricVals.begin(); it != correctMetricVals.end(); ++it)
            if (seenMetrics.find(it->first) == seenMetrics.end())
                Cout << "Error: missing metric value - " << it->first << Endl;


    }
    return 0;
}

}

int main(int argc, char** argv) {
    return NSnippets::Run(argc, argv);
}
