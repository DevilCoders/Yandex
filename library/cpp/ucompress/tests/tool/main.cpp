#include <library/cpp/ucompress/reader.h>
#include <library/cpp/ucompress/writer.h>

#include <library/cpp/blockcodecs/codecs.h>
#include <library/cpp/getopt/small/last_getopt.h>

#include <util/stream/file.h>


int main(int argc, const char** argv) {
    bool doCompress = false;
    bool doDecompress = false;
    TString codec;
    TString fromFileName;
    TString toFileName;

    NLastGetopt::TOpts options;
    options.AddCharOption('c', "Compress").StoreTrue(&doCompress);
    options.AddCharOption('d', "Decompress").StoreTrue(&doDecompress);
    options.AddCharOption('C', "Codec for compression").StoreResult(&codec);
    options.AddCharOption('f', "Source file").Required().StoreResult(&fromFileName);
    options.AddCharOption('t', "Destination file").Required().StoreResult(&toFileName);
    options.SetFreeArgsMax(0);
    NLastGetopt::TOptsParseResult optParseResult(&options, argc, argv);

    if ((doCompress == doDecompress) || (doCompress && !codec) || (doDecompress && codec)) {
        options.PrintUsage(argv[0]);
        return -1;
    }

    TFileInput fromIn(fromFileName);
    TFileOutput toOut(toFileName);

    if (doCompress) {
        NUCompress::TCodedOutput cOut(&toOut, NBlockCodecs::Codec(codec));
        fromIn.ReadAll(cOut);
    } else {
        NUCompress::TDecodedInput dIn(&fromIn);
        dIn.ReadAll(toOut);
    }

    return 0;
}
