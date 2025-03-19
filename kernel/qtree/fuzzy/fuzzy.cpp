#include "fuzzy.h"

#include <kernel/qtree/richrequest/serialization/serializer.h>
#include <kernel/proto_fuzz/fuzzy_output.h>

#include <library/cpp/string_utils/base64/base64.h>

void FuzzRichTree(
    NProtoBuf::Message& proto,
    const NProtoFuzz::TFuzzer::TOptions& opts)
{
    NProtoFuzz::TFuzzer fuzzer(opts);
    fuzzer.Fuzz(proto);
}

void FuzzBinaryRichTree(
    TString& binary,
    const NProtoFuzz::TFuzzer::TOptions& opts,
    const TRichTreeFuzzOptions& fuzzOpts)
{
    NProtoFuzz::TFuzzer::TRng rng(opts.Seed);

    NRichTreeProtocol::TRichRequestNode proto;
    bool canDeserialize = false;

    try {
        if (!binary.empty()) {
            TRichTreeDeserializer().DeserializeToProto(binary, false, proto);
        }
        canDeserialize = true;
    } catch (yexception&) {
    }

    if (canDeserialize) {
        FuzzRichTree(proto, opts.ReSeed(rng.GenRand64()));

        binary = TString();
        TCompressorFactory::EFormat format = NProtoFuzz::TFuzzer::TValues::GenEnum(rng,
            {TCompressorFactory::NO_COMPRESSION,
            TCompressorFactory::LZ_LZ4,
            TCompressorFactory::ZLIB_DEFAULT,
            TCompressorFactory::GZIP_DEFAULT,
            TCompressorFactory::BC_ZSTD_08_1});

        TRichTreeSerializer().SerializeFromProto(proto, false, binary, format);
    }

    if (fuzzOpts.FuzzBinary && 0 == (rng.Uniform(8))) {
        // fuzz binary bits
        NProtoFuzz::TFuzzyOutput::TOptions outOpts;
        outOpts.PartSize = 64;
        NProtoFuzz::FuzzBits(binary, outOpts.ReSeed(rng.GenRand64()));
    }
}

void FuzzQTree(
    TString& qtree,
    const NProtoFuzz::TFuzzer::TOptions& opts,
    const TRichTreeFuzzOptions& fuzzOpts)
{
    NProtoFuzz::TFuzzer::TRng rng(opts.Seed);

    TString binary;

    bool canParse = false;

    try {
        binary = Base64Decode(qtree);
        canParse = true;
    } catch (yexception&) {
    }

    if (canParse) {
        FuzzBinaryRichTree(binary, opts.ReSeed(rng.GenRand64()), fuzzOpts);
        qtree = TString();
        Base64EncodeUrl(binary, qtree);
    }

    if (fuzzOpts.FuzzBase64 && 0 == rng.Uniform(16)) {
        // fuzz base64 bits
        NProtoFuzz::TFuzzyOutput::TOptions outOpts;
        outOpts.PartSize = 100;
        NProtoFuzz::FuzzBits(qtree, outOpts.ReSeed(rng.GenRand64()));
    }
}
