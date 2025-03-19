#pragma once

#include <kernel/proto_fuzz/fuzzer.h>

#include <kernel/reqbundle/proto/reqbundle.pb.h>

namespace NReqBundle {
    struct TFuzzOptions {
        bool FuzzBinary = true;
        bool FuzzBase64 = true;
    };

    void FuzzReqBundle(
        NProtoBuf::Message& proto,
        const NProtoFuzz::TFuzzer::TOptions& opts = {});

    void FuzzBinary(
        TString& binary,
        const NProtoFuzz::TFuzzer::TOptions& opts = {},
        const TFuzzOptions& fuzzOpts = {});

    void FuzzQBundle(
        TString& qbundle,
        const NProtoFuzz::TFuzzer::TOptions& opts = {},
        const TFuzzOptions& fuzzOpts = {});
} // NReqBundle
