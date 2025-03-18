#include <yweb/structhtml/htmlstatslib/htmlstatslib.h>

#include <tools/snipmake/contentstats/util/kiwi.h>

#include <library/cpp/getopt/last_getopt.h>
#include <library/cpp/json/writer/json.h>
#include <contrib/libs/openssl/include/openssl/sha.h>
#include <util/generic/strbuf.h>
#include <util/generic/string.h>
#include <util/stream/str.h>
#include <util/stream/file.h>
#include <util/stream/output.h>
#include <util/stream/format.h>
#include <util/folder/path.h>

TString GetContentHash(const TStringBuf& buf)
{
    SHA256_CTX Ctx;
    SHA256_Init(&Ctx);
    SHA256_Update(&Ctx, buf.data(), buf.size());
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256_Final(hash, &Ctx);
    TStringStream stream;
    for (size_t i = 0; i < SHA256_DIGEST_LENGTH; ++i) {
        stream << Hex(hash[i], HF_FULL);
    }
    return stream.Str();
}

void AppendJsonLog(const TString& url, const TString& hash, ECharset encoding, IOutputStream& out)
{
    NJsonWriter::TBuf answer;
    answer.BeginObject();
    answer.WriteKey("url").WriteString(url);
    answer.WriteKey("final_url").WriteString(url);
    answer.WriteKey("encoding").WriteInt(encoding);
    answer.WriteKey("mime_type").WriteString("text/html");
    answer.WriteKey("content_hash").WriteString(hash);
    answer.WriteKey("http_code").WriteInt(200);
    answer.WriteKey("failed").WriteString("false");
    answer.WriteKey("error_message").WriteString("");
    answer.EndObject();
    out << answer.Str() << Endl;
}

void Process(IInputStream* inStream, const TFsPath& outDirectory, int dilution)
{
    int idx = 0;
    NHtmlStats::TUrlGrouper grouper;
    grouper.LoadGroups("mined_groups.txt");
    TUnbufferedFileOutput manifestFile(outDirectory / "manifest.json");

    NHtmlStats::TProtoStreamIterator iter(inStream);
    NHtmlStats::TProtoDoc doc;

    for (; iter.Next(doc); ++idx) {
        if (!doc.Url || !doc.Html) {
            continue;
        }

        if (idx % dilution != 0) {
            continue;
        }

        TString hash = GetContentHash(doc.Html);
        {
            TFixedBufferFileOutput outFile(outDirectory / hash);
            outFile << doc.Html;
        }
        AppendJsonLog(doc.Url, hash, doc.Encoding, manifestFile);
        Cout << grouper.GetGroupName(doc.Url) << "\t" << doc.Url << Endl;
    }
}

int main(int argc, char** argv) {
    TString urlFileName;
    TString outDirectoryPath;
    int dilution = 1;

    using namespace NLastGetopt;
    TOpts opt;
    opt.AddCharOption('f', REQUIRED_ARGUMENT, "file with protobin record stream").StoreResult(&urlFileName);
    opt.AddCharOption('d', REQUIRED_ARGUMENT, "path to output directory").StoreResult(&outDirectoryPath).DefaultValue(".cache");
    opt.AddCharOption('p', REQUIRED_ARGUMENT, "unpack each p-th document").StoreResult(&dilution).DefaultValue("1");

    TOptsParseResult o(&opt, argc, argv);

    THolder<IInputStream> input;
    if (!!urlFileName) {
        input.Reset(new TFileInput(urlFileName));
    }
    else {
        input.Reset(new TBufferedInput(&Cin));
    }
    Process(input.Get(), TFsPath(outDirectoryPath), dilution);
}
