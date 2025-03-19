#pragma once

#include <kernel/proto_fuzz/fuzzer.h>

#include <kernel/qtree/richrequest/protos/rich_tree.pb.h>

struct TRichTreeFuzzOptions {
    bool FuzzBinary = true;
    bool FuzzBase64 = true;
};

void FuzzRichTree(
    NProtoBuf::Message& proto,
    const NProtoFuzz::TFuzzer::TOptions& opts = {});

void FuzzBinaryRichTree(
    TString& binary,
    const NProtoFuzz::TFuzzer::TOptions& opts = {},
    const TRichTreeFuzzOptions& fuzzOpts = {});

void FuzzQTree(
    TString& qtree,
    const NProtoFuzz::TFuzzer::TOptions& opts = {},
    const TRichTreeFuzzOptions& fuzzOpts = {});
