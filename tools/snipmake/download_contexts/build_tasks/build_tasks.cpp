#include <util/random/shuffle.h>
#include <library/cpp/getopt/opt.h>
#include <util/charset/wide.h>
#include <util/generic/algorithm.h>
#include <util/generic/hash.h>
#include <util/generic/set.h>
#include <util/generic/vector.h>
#include <util/stream/file.h>
#include <util/string/vector.h>
#include <util/string/split.h>

int main(int argc, char* argv[]) {
    const char* usage =
       "Usage: build_tasks [i:o:n:d:u] \n"
            "-i <input file>         --path to input file, required\n"
            "-o <output file>        --path to output file, required\n"
            "-n <number of tasks>    --default 1000000\n"
            "-d <domain>             --domain ru, ua, com.tr, etc.\n"
            "-u                      --if it asks for unique queries and urls\n";

    Opt opt(argc, argv, "i:o:n:d:u");
    int optcode = EOF;

    TString inputFile;
    TString outputFile;
    size_t numberOfTasks = 100000;
    TString domain;
    bool unique = false;

    bool opterr = false;
    while ((optcode = opt.Get()) != EOF) {
        switch (optcode) {
        case 'i':
            inputFile = opt.Arg;
            break;
        case 'o':
            outputFile = opt.Arg;
            break;
        case 'n':
            numberOfTasks = FromString<size_t>(opt.Arg);
            break;
        case 'd':
            domain = opt.Arg;
            break;
        case 'u':
            unique = true;
            break;
        default: opterr = true;
            break;
        }
    }

    if (opterr || inputFile.empty() || outputFile.empty()) {
        errx(1, "%s", usage);
    }

    TFileInput file(inputFile);
    TVector<TString> lines;
    TString line;
    while (file.ReadLine(line)) {
        lines.push_back(line);
    }
    for (size_t iter = 0; iter < 10; ++iter)
         Shuffle(lines.begin(), lines.end());
    TOFStream out(outputFile);
    size_t total = 0;
    TSet<TString> queries;
    TSet<TString> urls;
    for (size_t num = 0; num < lines.size(); ++num) {
        TVector<TString> task;
        StringSplitter(lines[num]).Split('\t').SkipEmpty().Collect(&task);
        if (task.size() >= 3) {
            TString query = task[0];
            TString regionId = task[1];
            TString url = task[2];
            if (unique && (queries.find(query) != queries.end() || urls.find(url) != urls.end()))
                continue;
            queries.insert(query);
            urls.insert(url);
            out << query << "\turl:" << url << "\t" << regionId;
            out << "\t" << domain;
            out << "\t" << (task.size() >= 5 ? task[4] : ""); //uil
            out << "\t" << (task.size() >= 4 ? task[3] : ""); //requestId
            out << Endl;
            ++total;
         }
         if (total == numberOfTasks)
             break;
    }
    return 0;
}
