#include <library/cpp/getopt/last_getopt.h>

#include <yweb/robot/kiwi/protos/kwworm.pb.h>

#include <mapreduce/library/io/streaming/streaming.h>
#include <mapreduce/library/io/stream/stream.h>

#include <util/generic/buffer.h>
#include <util/stream/file.h>
#include <util/system/backtrace.h>

#include <google/protobuf/messagext.h>


struct TConfig {
    TString InFile = "/dev/stdin";
    TString OutFile = "/dev/stdout";
    TString SubKey;

    TConfig(int argc, const char* argv[]) {
        NLastGetopt::TOpts opts;

        opts.AddLongOption('i', "in", "input file")
            .RequiredArgument("FILE")
            .DefaultValue(InFile)
            .StoreResult(&InFile);

        opts.AddLongOption('o', "out", "output file")
            .RequiredArgument("FILE")
            .DefaultValue(OutFile)
            .StoreResult(&OutFile);

        opts.AddLongOption(
                's', "subkey", "store this value as a subkey (switches the output format to application/x-yamr-subkey-lenval)"
            )
            .RequiredArgument("STRING")
            .Optional()
            .StoreResult(&SubKey);

        opts.AddHelpOption();

        NLastGetopt::TOptsParseResult(&opts, argc, argv);
    }
};

bool ReadProto(google::protobuf::io::TCopyingInputStreamAdaptor& in, NKiwiWorm::TRecord& proto) {
    using namespace ::google::protobuf;

    io::CodedInputStream decoder(&in);
    decoder.SetTotalBytesLimit(512 * 1024  * 1024);

    return io::ParseFromCodedStreamSeq(&proto, &decoder);
}

void Protobin2YaMRLenval(IInputStream& in, IOutputStream& out, const TString& subKey) {
    using namespace ::google::protobuf;
    io::TCopyingInputStreamAdaptor input(&in);

    const bool useSubKeys = ! subKey.empty();
    const bool useValues = true;
    TLenvalStreamingUpdate output(out, useSubKeys, useValues);

    NKiwiWorm::TRecord proto;
    while (ReadProto(input, proto)) {
        TValueOutput protoBin;
        {
           io::TCopyingOutputStreamAdaptor adaptor(&protoBin);
           io::SerializeToZeroCopyStreamSeq(&proto, &adaptor);
        }
        if (useSubKeys) {
            output.AddSub(proto.GetKey(), subKey, protoBin);
        } else {
            output.Add(proto.GetKey(), protoBin);
        }
    }
}

void Main(TConfig& config) {
    TFileInput input(config.InFile);
    TFixedBufferFileOutput output(config.OutFile);
    Protobin2YaMRLenval(input, output, config.SubKey);
}

int main(int argc, const char* argv[]) {
    TConfig config(argc, argv);

    // This is to prevent SIGABRT and coredumps
    try {
        Main(config);
    } catch (...) {
        PrintBackTrace();
        return EXIT_FAILURE;
    };

    return EXIT_SUCCESS;
}
