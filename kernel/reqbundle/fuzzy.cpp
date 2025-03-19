#include "fuzzy.h"

#include "serializer.h"

#include <kernel/proto_fuzz/fuzzy_output.h>
#include <kernel/proto_fuzz/fuzz_policy.h>

#include <library/cpp/string_utils/base64/base64.h>

#include <util/generic/bitops.h>

namespace {
    using namespace NReqBundle;

    class TReqBundleFuzzPolicy
        : public NProtoFuzz::TDefaultFuzzPolicy
    {
    public:
        void DoFuzzString(
            const NProtoBuf::FieldDescriptor& fd,
            TString& value) override
        {
            Y_ASSERT(!!GetRng());

            TStringBuf name{fd.name()};

            // if (TStringBuf("BinaryData") == name) { // fuzz inside binary blocks
            //     NReqBundle::FuzzBinary(
            //         value,
            //         GetFuzzerOptions().ReSeed(GetRng()->GenRand64()));
            // } else {
                NProtoFuzz::TDefaultFuzzPolicy::DoFuzzString(fd, value);
            // }
        }
    };
} // namespace

namespace NReqBundle {
    void FuzzReqBundle(
        NProtoBuf::Message& proto,
        const NProtoFuzz::TFuzzer::TOptions& opts)
    {
        NProtoFuzz::TFuzzer fuzzer(opts, MakeHolder<TReqBundleFuzzPolicy>());
        fuzzer.Fuzz(proto);
    }

    void FuzzBinary(
        TString& binary,
        const NProtoFuzz::TFuzzer::TOptions& opts,
        const TFuzzOptions& fuzzOpts)
    {
        NProtoFuzz::TFuzzer::TRng rng(opts.Seed);

        NReqBundleProtocol::TReqBundle proto;
        bool canDeserialize = false;

        try {
            if (!binary.empty()) {
                TReqBundleDeserializer::TOptions deserOpts;
                deserOpts.FailMode = TReqBundleDeserializer::EFailMode::SkipOnError;
                TReqBundleDeserializer deser(deserOpts);
                deser.DeserializeToProto(binary, proto);
            }
            canDeserialize = true;
        } catch (yexception&) {
        }

        if (canDeserialize) {
            FuzzReqBundle(proto, opts.ReSeed(rng.GenRand64()));
            TReqBundleSerializer ser;
            binary = ser.SerializeProto(proto);
        }

        if (false && fuzzOpts.FuzzBinary && 0 == (rng.Uniform(8))) {
            // fuzz binary bits
            NProtoFuzz::TFuzzyOutput::TOptions outOpts;
            outOpts.PartSize = 64;
            NProtoFuzz::FuzzBits(binary, outOpts.ReSeed(rng.GenRand64()));
        }
    }

    void FuzzQBundle(
        TString& qbundle,
        const NProtoFuzz::TFuzzer::TOptions& opts,
        const TFuzzOptions& fuzzOpts)
    {
        NProtoFuzz::TFuzzer::TRng rng(opts.Seed);

        TString binary;
        NReqBundleProtocol::TReqBundle proto;

        bool canParse = false;

        try {
            binary = Base64StrictDecode(qbundle);
            canParse = true;
        } catch (yexception&) {
        }

        if (canParse) {
            FuzzBinary(binary, opts.ReSeed(rng.GenRand64()), fuzzOpts);
            qbundle = TString();
            Base64EncodeUrl(binary, qbundle);
        }

        if (false && fuzzOpts.FuzzBase64 && 0 == rng.Uniform(16)) {
            // fuzz base64 bits
            NProtoFuzz::TFuzzyOutput::TOptions outOpts;
            outOpts.PartSize = 100;
            NProtoFuzz::FuzzBits(qbundle, outOpts.ReSeed(rng.GenRand64()));
        }
    }
} // NReqBundle
