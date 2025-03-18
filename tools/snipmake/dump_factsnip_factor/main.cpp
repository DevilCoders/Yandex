#include <kernel/snippets/factors/factors.h>

#include <library/cpp/getopt/last_getopt.h>
#include <library/cpp/string_utils/base64/base64.h>

#include <util/stream/zlib.h>
#include <util/stream/file.h>
#include <util/string/strip.h>

float GetFactorValue(TString compressedFactors, size_t neededFactorIndex) {
     TString compressedData(Base64Decode(compressedFactors));

    TStringInput compressedDataStream(compressedData);
    TZLibDecompress decompressor(&compressedDataStream, ZLib::ZLib);

    float value = 0;
    bool success = true;
    for (size_t snippetFactorIndex = 0; snippetFactorIndex < neededFactorIndex + 1; ++snippetFactorIndex) {
        success = (decompressor.Load(&value, sizeof(float)) == sizeof(float));
    }

    Y_ENSURE(success, "Critical issue while decompressing factors, probably invalid factor index");

    return value;
}

int main(int argc, const char* argv[]) {
    TString input;
    TString output;
    size_t factorIndex = 0;

    NLastGetopt::TOpts options;
    options.AddLongOption('i', "input", "Compressed snippet factors, base64, one per line").Required().StoreResult(&input);
    options.AddLongOption('o', "output", "Factor values").Required().StoreResult(&output);
    options.AddLongOption('n', "index", "Factor index. Default: the factoid DSSM factor index. Change it at your own risk").
         DefaultValue(size_t(NSnippets::A2_FQ_RU_FACT_SNIPPET_DSSM_FACTOID_SCORE)).StoreResult(&factorIndex);

    options.AddHelpOption();

    NLastGetopt::TOptsParseResult optsResult(&options, argc, argv);

    TFileInput inputStream(input);
    TFileOutput outputStream(output);
    TString line;

    while (inputStream.ReadLine(line)) {
        line = StripString(line);
        outputStream << GetFactorValue(line, factorIndex) << Endl;
    }

    return 0;
}
