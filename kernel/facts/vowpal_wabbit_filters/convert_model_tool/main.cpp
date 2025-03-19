#include <library/cpp/vowpalwabbit/vowpal_wabbit_model.h>
#include <library/cpp/vowpalwabbit/vowpal_wabbit_predictor.h>
#include <library/cpp/getopt/small/last_getopt.h>


int main(int argc, const char* argv[]) {
    TString readableModelPath;
    TString convertedModelPath;

    NLastGetopt::TOpts options;

    options
        .AddLongOption("readable-model", "Readable model from `--readable_model` vw cli")
        .Required()
        .RequiredArgument("READABLE_MODEL")
        .StoreResult(&readableModelPath);

    options
        .AddLongOption("converted-model", "Converted model")
        .Required()
        .RequiredArgument("CONVERTED_MODEL")
        .StoreResult(&convertedModelPath);

    NLastGetopt::TOptsParseResult opts(&options, argc, argv);

    NVowpalWabbit::TModel::ConvertReadableModel(readableModelPath, convertedModelPath);
    Cout << "Converted model `" << readableModelPath << "` to `" << convertedModelPath << "`" << Endl;

    return 0;
}
