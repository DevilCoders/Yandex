#include "recognizers_shell.h"

#include <dict/recognize/docrec/dumper.h>
#include <dict/recognize/docrec/settings.h>

#include <library/cpp/getopt/opt.h>

#include <util/datetime/base.h>
#include <util/folder/dirut.h>
#include <util/generic/string.h>
#include <util/stream/file.h>
#include <util/stream/output.h>
#include <util/string/cast.h>
#include <util/string/vector.h>

using namespace NRecognizer;

void Usage(const char* progname) {
    TString shortname = progname;
    size_t pos = shortname.find_last_of("\\/:");
    if (pos != TString::npos)
        pos = shortname.find_first_not_of("\\/:", pos);
    if (pos != TString::npos)
        shortname = shortname.substr(pos);

    Cerr << "Usage: " << shortname << " [options] <dict file> [<input file or directory>]" << Endl;
    Cerr << "Recognizes text language and encoding" << Endl;
    Cerr << "Options:" << Endl;
    Cerr << "    -h, -?      Print this synopsis and exit" << Endl;
    Cerr << "    -d          use dict recognizer (default)"  << Endl;
    Cerr << "    -p          use html parser"  << Endl;
    Cerr << "    -u          accept ucs-2le input"  << Endl;
    Cerr << "    -i          Display supported encodings and languages" << Endl;
    Cerr << "    -t          Display processing time" << Endl;
    Cerr << "    -f          Take files list from input" << Endl;
    Cerr << "    -a          Print all languages" << Endl;
    Cerr << "    -l          Treat each line of input as a separate document" << Endl;
    Cerr << "    -b <size>   Set buffer size limit" << Endl;
    Cerr << "    -z <int>    Dump some statistics" << Endl;
    Cerr << "    -r          Force result printing" << Endl;
    Cerr << "    -q          Learning dump path" << Endl;
    Cerr << "    -y <domain> Use specified url by default" << Endl;
    Cerr << "If no input file or directory is specified, data is read from stdin." << Endl;
}

int main(int argc, char* argv[]) {
    Opt opt(argc, argv, "rs:z:dhpuifaltb:q:y:");
    int optcode = EOF;

    TRecognizersShell recognizers;

    TString learningFile;
    bool showInfo = false;
    bool timed = false;
    bool listOfFiles = false;
    bool forceRes = false;

    while ((optcode = opt.Get()) != EOF) {
        switch (optcode) {
        case '?':
        case 'h':
            Usage(argv[0]);
            return 0;
        case 'd':
            recognizers.Recognizer = TRecognizersShell::DictRec;
            break;
        case 'l':
            recognizers.LineByLine = true;
            break;
        case 'p':
            recognizers.UseParser = true;
            break;
        case 'u':
            recognizers.UseUCS2 = true;
            break;
        case 'i':
            showInfo = true;
            break;
        case 't':
            timed = true;
            break;
        case 'f':
            listOfFiles = true;
            break;
        case 'a':
            recognizers.AllLanguages = true;
            break;
        case 'z':
            Singleton<TDumper>()->SetDumpType(static_cast<TDumper::EDumpType>(FromString<int>(opt.Arg)));
            recognizers.PrintResults = false;
            break;
        case 'b':
            recognizers.BufferSize = FromString<size_t>(opt.Arg);
            break;
        case 'r':
            forceRes = true;
            break;
        case 'q':
            learningFile = opt.Arg;
            break;
        case 'y':
            recognizers.DefaultUrl = opt.Arg;
            break;
        }
    }

    recognizers.PrintResults = recognizers.PrintResults || forceRes;

    if (opt.Ind >= argc) {
        Cerr << "No dictionary file is specified" << Endl << Endl;
        Usage(argv[0]);
        return 1;
    }

    if (recognizers.Recognizer == TRecognizersShell::UndefinedRec) {
        Cerr << "No recognizer is specified" << Endl;
        Cerr << Endl;

        Usage(argv[0]);
        return 1;
    }

    TString dictFile = argv[opt.Ind];
    TString inPath = (opt.Ind + 1 < argc) ? argv[opt.Ind + 1] : "";

    recognizers.Load(dictFile);
    if (showInfo) {
        recognizers.ShowInfo();
        return 0;
    }

    bool isDir = IsDir(inPath);

    timeval starttime;
    memset(&starttime, 0, sizeof(timeval));
    if (timed) {
        gettimeofday(&starttime, nullptr);
    }

    if (recognizers.LineByLine && isDir) {
        Cerr << "Line by line mode can not be used for directories" << Endl;
        return 1;
    }

    if (isDir) {
        ProcessDirectory(inPath, recognizers);
    } else if (NFs::Exists(inPath)) {
        if (listOfFiles) {
            ProcessListOfFiles(inPath, recognizers);
        } else {
            recognizers.ProcessFile(inPath);
        }
    } else if (!inPath.empty()) {
        Cerr << "Invalid input path" << Endl;
        return 1;
    } else {
        if (listOfFiles) {
            ProcessListOfFiles(Cin, recognizers);
        } else {
            recognizers.Process(Cin);
        }
    }

    if (timed) {
        timeval endtime;
        gettimeofday(&endtime, nullptr);

        double timediff = endtime.tv_sec - starttime.tv_sec + 0.000001 * (endtime.tv_usec - starttime.tv_usec);
        fprintf(stderr, "Time elapsed: %.3f s\n", timediff);
    }

    return 0;
}
