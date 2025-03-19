#include <kernel/ethos/lib/autoencoder/pca.h>

#include <library/cpp/getopt/last_getopt.h>
#include <library/cpp/getopt/modchooser.h>

#include <util/stream/file.h>
#include <util/string/join.h>

template <typename TEncoder>
int ApplyMode(int argc, const char** argv) {
    TString featuresPath;
    TString encoderModelPath;

    {
        NLastGetopt::TOpts opts;
        opts.SetFreeArgsMax(0);

        opts.AddLongOption('f', "features", "features path")
            .Required()
            .StoreResult(&featuresPath);
        opts.AddLongOption('m', "model", "resulting encoder model path")
            .Required()
            .StoreResult(&encoderModelPath);

        NLastGetopt::TOptsParseResult res(&opts, argc, argv);
    }

    TEncoder encoder;
    {
        TFileInput modelIn(encoderModelPath);
        encoder.Load(&modelIn);
    }

    PrintFeatureTransformations(featuresPath, encoder);

    return 0;
}

int LearnMode(int argc, const char** argv) {
    TModChooser modChooser;
    modChooser.AddMode("pca", NEthosAutoEncoder::LearnPCA, "learn pca features encoder");
    return modChooser.Run(argc, argv);
}

int ApplyMode(int argc, const char** argv) {
    TModChooser modChooser;
    modChooser.AddMode("pca", ApplyMode<NEthosAutoEncoder::TLinearFeaturesEncoder>, "apply pca features encoder");
    return modChooser.Run(argc, argv);
}

int main(int argc, const char** argv) {
    TModChooser modChooser;
    modChooser.AddMode("learn", LearnMode, "learn encoder mode");
    modChooser.AddMode("apply", ApplyMode, "apply encoder mode");
    return modChooser.Run(argc, argv);
}
