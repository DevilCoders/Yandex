#include <tools/crx_creator/lib/crx_creator.h>

#include <library/cpp/getopt/last_getopt.h>

#include <util/generic/string.h>
#include <util/memory/blob.h>
#include <util/stream/file.h>
#include <util/system/file.h>

using namespace NLastGetopt;

int main(int argc, char* argv[]) {
    TOpts opts;
    opts.AddLongOption('i', "input", "- Input zip archive").Required();
    opts.AddLongOption('k', "key", "- Private RSA key in pem format").Required();
    opts.AddLongOption('o', "output", "- Output crx file").Required();
    TOptsParseResult optsResult(&opts, argc, argv);

    TBlob header;
    {
        TBlob pem = TBlob::FromFile(optsResult.Get("key"));
        TIFStream archive(optsResult.Get("input"));
        header = GetCrx3HeaderRSA(pem, archive);
    }

    TIFStream archive(optsResult.Get("input"));
    TOFStream output(optsResult.Get("output"));
    output.Write(header.Data(), header.Length());
    const size_t bufferSize = 1 << 16;
    TArrayHolder<char> buffer(new char[bufferSize]);
    while (true) {
        size_t realSize = archive.Read(buffer.Get(), bufferSize);
        if (realSize == 0) {
            break;
        }
        output.Write(buffer.Get(), realSize);
    }
    output.Finish();
    return 0;
}
