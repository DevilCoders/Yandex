#include <library/cpp/blockcodecs/codecs.h>
#include <library/cpp/blockcodecs/stream.h>
#include <library/cpp/streams/factory/factory.h>

#include <util/stream/file.h>
#include <util/stream/output.h>
#include <util/memory/blob.h>

using namespace NBlockCodecs;

namespace {
    static inline const ICodec* CodecX(TStringBuf name) {
        return Codec(name);
    }
}

int main(int argc, char** argv) {
    if (argc < 5) {
        return -1;
    }

    TStringBuf mode = argv[1];

    auto codec = CodecX(argv[2]);
    auto fo = OpenOutput(argv[4]);

    if (mode.StartsWith("stream-")) {
        mode = mode.SubStr(7);

        auto fi = OpenInput(argv[3]);

        if (mode == "compress"sv) {
            TCodedOutput out(fo.Get(), codec, 1000000);

            fi->ReadAll(out);
            out.Finish();
        } else {
            TDecodedInput(fi.Get()).ReadAll(*fo);
        }
    } else {
        auto data = TBlob::FromFile(argv[3]);
        auto res = (mode == "compress"sv) ? codec->Encode(data) : codec->Decode(data);

        fo->Write(res.data(), res.size());
    }

    fo->Finish();
}
