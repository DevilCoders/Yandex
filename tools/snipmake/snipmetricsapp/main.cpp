#include <tools/snipmake/snipmetrics/avecalc.h>
#include <tools/snipmake/snipmetrics/calculators.h>
#include <tools/snipmake/snipmetrics/dump.h>
#include <tools/snipmake/snipmetrics/snip2serpiter.h>
#include <tools/snipmake/snipmetrics/snipiter.h>
#include <tools/snipmake/snipmetrics/snipmetrics.h>
#include <tools/snipmake/snipmetrics/wizserp.h>
#include <tools/snipmake/snippet_xml_parser/cpp_reader/snippet_xml_reader.h>

#include <library/cpp/getopt/opt.h>

#include <util/generic/ptr.h>

namespace NSnippets {

const TString APP_DUMP       = "dump_metrics";
const TString APP_AVE        = "calc_metrics_average";
const TString APP_SERP       = "serp_metrics";
const TString INPMODE_TAB    = "tab";
const TString INPMODE_XML    = "xml";
const TString INPMODE_SERP   = "serp";
const TString OUTMODE_PRINT  = "print";
const TString OUTMODE_XML    = "xml";
const TString OUTMODE_STAT   = "stat";

void PrintUsage() {
    Cout << "snipmetricsapp [options]" << Endl;
    Cout << "  Prints to stdout snippet metrics." << Endl;
    Cout << "Options: \n"
            "  -i input file\n"
            "      path to snippets file, if not set - reads data from stdin\n"
            "  -a appname\n"
            "      values are 'dump_metrics' , 'calc_metrics_average' or 'serp_metrics', default is 'dump_metrics'\n"
            "        calc_metrics_average - calculate average metrics for all metrics\n"
            "        dump_metrics - dump snippet metrics for all snippets\n"
            "        serp_metrics - calculate average metrics for serp (resets input mode in 'serp')\n"
            "  -d path to stopword.lst\n"
            "      must be checked out from 'svn+ssh://arcadia.yandex.ru/arc/trunk/arcadia_tests_data/wizard/language/stopword.lst'\n"
            "  -p path to porno words config\n"
            "      place in arcadia is 'yweb/pornofilter/porno_config.dat'\n"
            "  -m input mode\n"
            "      values are 'tab' or 'xml', 'serp', default is 'tab'\n"
            "  -o output mode for 'dump_metrics' app\n"
            "      values are 'print', 'xml' or 'stat', 'point' for serp_metrics, default is 'print'\n"
            "  -w wizards file\n"
            "      path to wizards file, needed in serp_metrics\n"
            "  -r point file\n"
            "      path to serp_metrics result file\n"
            "  -h or -? current help\n"
            << Endl;
}

int Run(int argc, char** argv) {
    if (argc < 2) {
        PrintUsage();
        return 1;
    }
    Opt opt(argc, argv, "i:a:d:p:m:o:w:r:h:");
    int optlet;

    // cmd options
    TString inFile = "";
    TString wizFile="";
    TString pntFile="";
    TString appName = APP_DUMP;
    TString pornoWordsConfig = "";
    TString stopwordsFile = "";
    TString inMode = INPMODE_TAB;
    TString outMode = OUTMODE_PRINT;
    while ((optlet = opt.Get()) != EOF) {
        switch (optlet) {
            case 'i':
                inFile = opt.Arg;
                break;
            case 'a':
                appName = opt.Arg;
                break;
            case 'd':
                stopwordsFile = opt.Arg;
                break;
            case 'p':
                pornoWordsConfig = opt.Arg;
                break;
            case 'm':
                inMode = opt.Arg;
                break;
            case 'o':
                outMode = opt.Arg;
                break;
            case 'w':
                wizFile = opt.Arg;
                break;
            case 'r':
                pntFile = opt.Arg;
                break;
            case 'h':
            case '?':
            default:
                PrintUsage();
                return 0;
                break;
        }
    }

    // check input parameters
    if (appName != APP_AVE && appName != APP_DUMP && appName != APP_SERP) {
        Cerr << "ERROR: Wrong app name!" << Endl;
        PrintUsage();
        return 1;
    }

    if (!pornoWordsConfig.size()) {
        Cerr << "ERROR: porno words config not set!" << Endl;
        PrintUsage();
        return 1;
    }

    if (!stopwordsFile.size()) {
        Cerr << "ERROR: stopwordsFile not set!" << Endl;
        PrintUsage();
        return 1;
    }

    if (inMode != INPMODE_TAB && inMode != INPMODE_XML && inMode!=INPMODE_SERP) {
        Cerr << "ERROR: wrong input mode!" << Endl;
        PrintUsage();
        return 1;
    }

    if (outMode != OUTMODE_PRINT && outMode != OUTMODE_XML && outMode != OUTMODE_STAT) {
        Cerr << "ERROR: wrong output mode!" << Endl;
        PrintUsage();
        return 1;
    }
    if (appName == APP_SERP) {
        inMode = INPMODE_SERP;
        if (!wizFile.size() || !pntFile.size()) {
            Cerr << "ERROR: serp_metrics mode needs wizards & output point filename"<< Endl;
            PrintUsage();
            return 1;
        }
    }
    // end check input parameters

    try {
        THolder<TUnbufferedFileInput> requests;
        IInputStream* inp = (inFile.size())? new TFileInput(inFile) : &Cin;
        IOutputStream* out = (pntFile.size())? new TFixedBufferFileOutput(pntFile) : &Cout;


        TString desc = "Query metrics";

        THolder<ISnippetsIterator> innerIterator;
        THolder<ISerpsIterator> iterator;
        if (inMode == INPMODE_TAB) {
            innerIterator.Reset(new TSnipRawIter(inp));
            iterator.Reset(new TSnip2SerpIter(innerIterator.Get()));
        } else if (inMode == INPMODE_XML) {
            innerIterator.Reset(new TSnippetsXmlIterator(inp));
            iterator.Reset(new TSnip2SerpIter(innerIterator.Get()));
        } else if (inMode == INPMODE_SERP) {
            requests.Reset(new TUnbufferedFileInput(wizFile));
            iterator.Reset(new TSerpXmlIterator(inp, requests.Get()));
        }

        THolder<TSnipMetricsCalculator> calc;
        if (appName == APP_AVE) {
            calc.Reset(new TSnipMetricsAveCalculator(desc, stopwordsFile, pornoWordsConfig, out));
        } else if (appName == APP_DUMP){
            if (outMode == OUTMODE_PRINT) {
                calc.Reset(new TSnipMetricsDumper<TSimpleSnippetDumper>(desc, stopwordsFile, pornoWordsConfig, out));
            } else if (outMode == OUTMODE_XML) {
                calc.Reset(new TSnipMetricsDumper<TXmlSnippetDumper>(desc, stopwordsFile, pornoWordsConfig, out));
            } else if (outMode == OUTMODE_STAT) {
                calc.Reset(new TSnipMetricsDumper<TSnipStatDumper>(desc, stopwordsFile, pornoWordsConfig, out));
            }
        } else if (appName == APP_SERP) {
            calc.Reset(new TMetricsAveCalc("SerpMetricsPoint", stopwordsFile, pornoWordsConfig, out));
        }

        TSnippetsMetricsCalculationApplication app(iterator.Get(), calc.Get());
        app.Run();

        if (inp != &Cin)
            delete inp;
        if (out != &Cout)
            delete out;
    } catch (const yexception& e) {
        Cerr << "snipmetricsapp error: " << e.what() << Endl;
    }

    return 0;
}

}

int main(int argc, char** argv) {
    return NSnippets::Run(argc, argv);
}
