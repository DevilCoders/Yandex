#include <yweb/structhtml/htmlstatslib/htmlstatslib.h>
#include <yweb/robot/kiwi/protos/kwworm.pb.h>

#include <tools/snipmake/contentstats/util/kiwi.h>

#include <library/cpp/getopt/last_getopt.h>
#include <google/protobuf/messagext.h>
#include <util/generic/strbuf.h>
#include <util/generic/string.h>
#include <util/stream/str.h>
#include <util/stream/file.h>
#include <util/stream/output.h>
#include <util/stream/format.h>
#include <util/random/shuffle.h>

void Process(IInputStream& inStream, IOutputStream& outStream, THashSet<TString>& foundUrls)
{
    using namespace google::protobuf::io;

    TCopyingOutputStreamAdaptor outfAdaptor(&outStream);

    NHtmlStats::TProtoStreamIterator iter(&inStream);
    NHtmlStats::TProtoDoc doc;

    while (iter.Next(doc)) {
        if (!doc.Url || !doc.Html) {
            continue;
        }
        foundUrls.insert(doc.Url);
        SerializeToZeroCopyStreamSeq(doc.Rec.Get(), &outfAdaptor);
    }
}

int main(int argc, char** argv) {
    TString inFileName;
    TString outFileName;

    using namespace NLastGetopt;
    TOpts opt;
    opt.AddCharOption('i', REQUIRED_ARGUMENT, "INPUT file with protobin record stream").StoreResult(&inFileName);
    opt.AddCharOption('o', REQUIRED_ARGUMENT, "OUTPUT file with good protobin records").StoreResult(&outFileName);

    TOptsParseResult o(&opt, argc, argv);

    if (!inFileName || !outFileName) {
        Cerr << "both the input (-i) and output (-o) files must be specified" << Endl;
    }

    THashSet<TString> inUrls;

    {
        TFileInput inf(inFileName, 1<<24);
        TUnbufferedFileOutput outf(outFileName);
        Process(inf, outf, inUrls);
        // Close the files so this program can be pipelined into kwworm read
    }

    Cerr << "Docs found in input file: " << inUrls.size() << Endl;
    Cerr.Flush();

    TBufferedInput bufCin(&Cin);
    TVector<TString> urlsToDownload;
    int srcUrls = 0;

    TString line;
    while (bufCin.ReadLine(line)) {
        ++srcUrls;
        if (!line || inUrls.contains(line)) {
            continue;
        }
        urlsToDownload.push_back(line);
    }

    Shuffle(urlsToDownload.begin(), urlsToDownload.end());

    Cerr << "URLs provided/left to download: " << srcUrls << "/" << urlsToDownload.size() << Endl;
    Cerr.Flush();

    for (const TString& url : urlsToDownload) {
        Cout << url << Endl;
    }
}
