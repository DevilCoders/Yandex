#include <util/random/shuffle.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <util/generic/yexception.h>
#include <util/stream/output.h>
#include <util/stream/input.h>
#include <util/stream/file.h>
#include <util/generic/algorithm.h>
#include <util/string/split.h>
#include <util/random/random.h>
#include <util/memory/blob.h>

#include <library/cpp/getopt/last_getopt.h>
#include <library/cpp/getopt/modchooser.h>
#include <library/cpp/json/json_reader.h>

#include <library/cpp/deprecated/omni/write.h>
#include <library/cpp/deprecated/omni/read.h>

namespace nlg = NLastGetopt;

using namespace NOmni;

int mode_write(const int argc, const char** argv) {
    nlg::TOpts opts;
    opts.AddHelpOption('h');

    TString schemePath;
    opts.AddLongOption("scheme-path", "path to scheme")
        .RequiredArgument("<scheme>")
        .Required()
        .StoreResult(&schemePath);

    TString dstPath;
    opts.AddLongOption("index-path", "path to dst index")
        .RequiredArgument("<index>")
        .Required()
        .StoreResult(&dstPath);

    const nlg::TOptsParseResult optsres(&opts, argc, argv);

    TFileArchiver indexer((TJsonFileScheme(schemePath)), dstPath);
    indexer.StartMap(0);
    for (int i = 0; i < 10; ++i) {
        indexer.StartMap(i);
        indexer.WriteData(0, FromString("hello world!"));
        indexer.StartMap(1);
        for (int j = 0; j < 3; ++j) {
            TString breakTxt = "break #" + ToString(j);
            indexer.WriteData(j, FromString(breakTxt));
        }
        indexer.EndMap(); //breaks
        indexer.EndMap(); //doc
    }
    indexer.EndMap(); //main docs
    indexer.StartMap(1);
    indexer.WriteData(0, FromString("14.07.1790"));
    indexer.WriteData(1, FromString("/place/home/snow"));
    indexer.EndMap(); // meta info
    indexer.Finish();

    return 0;
}

void MapUrlArchiveMode(IStackIndexer* indexer, const TVector<std::pair<size_t, TString>>& docs) {
    indexer->StartMap(0);
    size_t n = docs.size();
    for (size_t i = 0; i < n; ++i) {
        if (i % 10000 == 0)
            Cout << "i: " << i << Endl; //FOR_DEBUG
        indexer->WriteData(docs[i].first, FromString(docs[i].second));
    }
    indexer->EndMap(); //main docs
    indexer->StartMap(1);
    indexer->WriteData(0, FromString("14.07.1790"));
    indexer->WriteData(1, FromString("/place/home/snow"));
    indexer->EndMap(); // meta info
}

TVector<std::pair<size_t, TString>> MakeRandomDocs(const TVector<TString>& urls) {
    TVector<std::pair<size_t, TString>> docs;
    for (size_t i = 0; i < urls.size(); ++i) {
        docs.push_back(std::make_pair(i, urls[i]));
    }
    Shuffle(docs.begin(), docs.end());
    TVector<std::pair<size_t, TString>> res;
    for (size_t i = 0; i < urls.size(); ++i) {
        if (rand() % 10 == 0)
            res.push_back(docs[i]);
    }
    return res;
}

TVector<TString> ReadUrls(const TString& srcPath) {
    TString line;
    TFileInput fin(srcPath);
    TVector<TString> res;
    while (fin.ReadLine(line)) {
        size_t colPos = line.find(':');
        Y_VERIFY(line[colPos + 1] == '\t');
        Y_VERIFY(colPos != TString::npos);
        size_t curDoc = FromString<size_t>(line.substr(0, colPos));
        Y_VERIFY(curDoc == res.size());
        TString url = line.substr(colPos + 2);
        res.push_back(url);
    }
    return res;
}

int mode_write_map(const int argc, const char** argv) {
    nlg::TOpts opts;
    opts.AddHelpOption('h');

    TString srcPath;
    opts.AddLongOption("src-path", "path to urls dump")
        .RequiredArgument("<path>")
        .Required()
        .StoreResult(&srcPath);

    TString schemePath;
    opts.AddLongOption("scheme-path", "path to scheme")
        .RequiredArgument("<scheme>")
        .Required()
        .StoreResult(&schemePath);

    TString dstPath;
    opts.AddLongOption("index-path", "path to dst index")
        .RequiredArgument("<index>")
        .Required()
        .StoreResult(&dstPath);

    const nlg::TOptsParseResult optsres(&opts, argc, argv);

    TVector<TString> urls = ReadUrls(srcPath);
    auto docs = MakeRandomDocs(urls);

    TStatCollector collector((TJsonFileScheme(schemePath)));
    MapUrlArchiveMode(&collector, docs);
    collector.Finish();
    Cout << "stat collection finished" << Endl;
    TVector<TBlob> dynamicTables = collector.GetComputedCodecTables();
    TFileArchiver indexer((TJsonFileScheme(schemePath)), dstPath, &dynamicTables);
    MapUrlArchiveMode(&indexer, docs);
    indexer.Finish();

    return 0;
}

TVector<TString> GenerateUrls(size_t numUrls) {
    TVector<TString> lines;
    for (size_t i = 0; i < numUrls; ++i) {
        size_t size = rand() % 32;
        TString res = "www.yandex.ru/yandsearch?text=";
        for (size_t j = 0; j < size; ++j) {
            res.push_back('a' + rand() % 25);
        }
        lines.push_back(res);
    }
    return lines;
}

//XXX should we require user ro write a callback function like this?
void ArchiveCallbackLoop(IStackIndexer* indexer, const TVector<TString>& urls) {
    indexer->StartMap(0);
    size_t n = urls.size();
    for (size_t i = 0; i < n; ++i) {
        indexer->StartMap(i);
        indexer->WriteData(0, FromString(urls[i]));
        indexer->WriteData(1, FromString("hello world!"));
        indexer->EndMap(); //doc
    }
    indexer->EndMap(); //main docs
    //indexer->StartMap(1);
    //indexer->WriteData(0, FromString("14.07.1790"));
    //indexer->WriteData(1, FromString("/place/home/snow"));
    //indexer->EndMap(); // meta info
}

int mode_write_adaptive(const int argc, const char** argv) {
    nlg::TOpts opts;
    opts.AddHelpOption('h');

    TString schemePath;
    opts.AddLongOption("scheme-path", "path to scheme")
        .RequiredArgument("<scheme>")
        .Required()
        .StoreResult(&schemePath);

    TString dstPath;
    opts.AddLongOption("index-path", "path to dst index")
        .RequiredArgument("<index>")
        .Required()
        .StoreResult(&dstPath);

    size_t numUrls = 0;
    opts.AddLongOption("num-urls", "num random urls to generate")
        .RequiredArgument("<number>")
        .DefaultValue("100000")
        .Optional()
        .StoreResult(&numUrls);

    const nlg::TOptsParseResult optsres(&opts, argc, argv);

    TVector<TString> urls = GenerateUrls(numUrls);

    TStatCollector collector((TJsonFileScheme(schemePath)));
    ArchiveCallbackLoop(&collector, urls);
    collector.Finish();
    Cout << "stat collection finished" << Endl;
    TVector<TBlob> dynamicTables = collector.GetComputedCodecTables();
    TFileArchiver indexer((TJsonFileScheme(schemePath)), dstPath, &dynamicTables);
    ArchiveCallbackLoop(&indexer, urls);
    indexer.Finish();
    return 0;
}

void DocUrlArchiveCallbackLoop(IStackIndexer* indexer, const TVector<TString>& urls) {
    indexer->StartMap(0);
    size_t n = urls.size();
    for (size_t i = 0; i < n; ++i) {
        if (i % 10000 == 0)
            Cout << "i: " << i << Endl; //FOR_DEBUG
        indexer->WriteData(i, FromString(urls[i]));
    }
    indexer->EndMap(); //main docs
    indexer->StartMap(1);
    indexer->WriteData(0, FromString("14.07.1790"));
    indexer->WriteData(1, FromString("/place/home/snow"));
    indexer->EndMap(); // meta info
}

int mode_write_docurl(const int argc, const char** argv) {
    nlg::TOpts opts;
    opts.AddHelpOption('h');

    TString srcPath;
    opts.AddLongOption("src-path", "path to urls dump")
        .RequiredArgument("<path>")
        .Required()
        .StoreResult(&srcPath);

    TString schemePath;
    opts.AddLongOption("scheme-path", "path to scheme")
        .RequiredArgument("<scheme>")
        .Required()
        .StoreResult(&schemePath);

    TString dstPath;
    opts.AddLongOption("index-path", "path to dst index")
        .RequiredArgument("<index>")
        .Required()
        .StoreResult(&dstPath);

    const nlg::TOptsParseResult optsres(&opts, argc, argv);

    TVector<TString> urls = ReadUrls(srcPath);

    TStatCollector collector((TJsonFileScheme(schemePath)));
    DocUrlArchiveCallbackLoop(&collector, urls);
    collector.Finish();
    Cout << "stat collection finished" << Endl;
    TVector<TBlob> dynamicTables = collector.GetComputedCodecTables();
    TFileArchiver indexer((TJsonFileScheme(schemePath)), dstPath, &dynamicTables);
    DocUrlArchiveCallbackLoop(&indexer, urls);
    indexer.Finish();

    return 0;
}

int mode_read(const int argc, const char** argv) {
    nlg::TOpts opts;
    opts.AddHelpOption('h');

    TString indexPath;
    opts.AddLongOption("index-path", "path to index")
        .RequiredArgument("<index>")
        .Required()
        .StoreResult(&indexPath);

    const nlg::TOptsParseResult optsres(&opts, argc, argv);

    TOmniReader reader(indexPath);
    TOmniIterator iter = reader.Root();
    iter.DbgPrint(Cout);

    return 0;
}

class TCustomUtf8Printer: public IDebugViewer {
protected:
    void GetDbgLines(TStringBuf data, TVector<TString>* lines) const override {
        lines->clear();
        lines->push_back("this is a custom utf8 printer!");
        StringSplitter(data).Split('\n').AddTo(lines);
    }
};

int mode_test_dbg_printer(const int argc, const char** argv) {
    nlg::TOpts opts;
    opts.AddHelpOption('h');

    TString indexPath;
    opts.AddLongOption("index-path", "path to index")
        .RequiredArgument("<index>")
        .Required()
        .StoreResult(&indexPath);

    const nlg::TOptsParseResult optsres(&opts, argc, argv);

    TMap<TString, IDebugViewer*> customViewers;
    TCustomUtf8Printer customPrinter;
    customViewers["Url"] = &customPrinter;

    TOmniReader reader(indexPath);
    TOmniIterator iter = reader.Root();
    iter.DbgPrint(Cout, &customViewers);

    return 0;
}

int main(const int argc, const char** argv) {
    srand(1);
    TModChooser modChooser;
    modChooser.AddMode("write", mode_write, "write index mode");
    modChooser.AddMode("write_map", mode_write_map, "write_map index mode");
    modChooser.AddMode("write_adaptive", mode_write_adaptive, "write index mode");
    modChooser.AddMode("write_docurl", mode_write_docurl, "write index.docurl mode");
    modChooser.AddMode("read", mode_read, "read index mode");
    modChooser.AddMode("test_dbg_printer", mode_test_dbg_printer, "test dbg printer");
    return modChooser.Run(argc, argv);
}
