#include <kernel/blender/factor_storage/serialization.h>

#include <library/cpp/getopt/last_getopt.h>
#include <library/cpp/scheme/scheme.h>
#include <library/cpp/streams/factory/factory.h>
#include <util/folder/dirut.h>
#include <util/generic/hash.h>
#include <util/generic/vector.h>

using namespace NLastGetopt;

void doCompress(const TString& inFile, const TString& outFile) {
    THolder<IInputStream> in = OpenInput(inFile);
    TString jsonFactors = in->ReadLine();
    NSc::TValue factors = NSc::TValue::FromJson(jsonFactors);
    TVector<float> staticFactors;
    NJsonConverters::FromTValue(factors["static_factors"], staticFactors, true);
    THashMap<TString, TVector<float>> dynamicFactors;
    NJsonConverters::FromTValue(factors["dynamic_factors"], dynamicFactors, true);
    TString compressedFactors;
    NBlender::NProtobufFactors::Compress(compressedFactors, &staticFactors, &dynamicFactors);
    THolder<IOutputStream> out = OpenOutput(outFile);
    *out << compressedFactors;
}

void doDecompress(const TString& inFile, const TString& outFile) {
    THolder<IInputStream> in = OpenInput(inFile);
    TString compressedFactors = in->ReadLine();
    TString error;
    TVector<float> staticFactors;
    THashMap<TString, TVector<float>> dynamicFactors;
    if (!NBlender::NProtobufFactors::Decompress(&staticFactors, &dynamicFactors, compressedFactors, &error)) {
        ythrow yexception() << "Error while decompressing: " << error;
    }
    NSc::TValue res;
    res["static_factors"].GetArrayMutable().assign(staticFactors.begin(), staticFactors.end());
    NSc::TValue& resDynamicFactors = res["dynamic_factors"];
    for (const auto& [key, value] : dynamicFactors) {
        resDynamicFactors[key].GetArrayMutable().assign(value.begin(), value.end());
    }
    THolder<IOutputStream> out = OpenOutput(outFile);
    res.ToJson(*out);
}

int main(int argc, const char* argv[])
{
    TString mode, inFile, outFile;
    TOpts opts = TOpts::Default();
    opts.AddLongOption("mode", "mode: \"compress\", \"decompress\"")
        .RequiredArgument("MODE").Required().StoreResult(&mode);
    opts.AddLongOption("in_file", "input file path")
        .RequiredArgument("IN_FILE").Required().StoreResult(&inFile);
    opts.AddLongOption("out_file", "output file path")
        .RequiredArgument("OUT_FILE").Required().StoreResult(&outFile);

    TOptsParseResult parseResult(&opts, argc, argv);

    NFs::EnsureExists(inFile.data());

    if (mode == "compress") {
        doCompress(inFile, outFile);
    } else if (mode == "decompress") {
        doDecompress(inFile, outFile);
    } else {
       ythrow yexception() << "Unknown mode: " << mode;
    }

    return 0;
}
