#include "wrapper.h"

#include <library/cpp/monlib/encode/json/json.h>
#include <library/cpp/monlib/encode/spack/spack_v1.h>

#include <util/stream/mem.h>
#include <util/stream/str.h>

using namespace NMonitoring;


TString ConvertJsonSensorsToSpackV1(TStringBuf jsonData) {
    TStringStream output;

    auto encoder = EncoderSpackV1(&output, ETimePrecision::SECONDS, ECompression::LZ4);
    DecodeJson(jsonData, encoder.Get());
    encoder->Close();

    return output.Str();
}

TString ConvertSpackV1SensorsToJson(TStringBuf spackData) {
    TMemoryInput input(spackData);
    TStringStream output;

    auto encoder = EncoderJson(&output, /* pretty = */ false);
    DecodeSpackV1(&input, encoder.Get());
    encoder->Close();

    return output.Str();
}
