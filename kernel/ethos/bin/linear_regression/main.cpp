#include <kernel/ethos/lib/cross_validation/cross_validation.h>
#include <kernel/ethos/lib/data/dataset.h>

#include <library/cpp/getopt/last_getopt.h>
#include <library/cpp/getopt/modchooser.h>

#include <library/cpp/linear_regression/linear_regression.h>
#include <library/cpp/linear_regression/linear_model.h>
#include <library/cpp/linear_regression/unimodal.h>
#include <library/cpp/linear_regression/welford.h>

#include <library/cpp/scheme/scheme.h>

#include <library/cpp/streams/factory/factory.h>

#include <util/string/join.h>
#include <util/string/printf.h>

#include <util/system/fs.h>
#include <util/system/yassert.h>

enum ELearningMode {
    LM_FAST_BSLR,
    LM_KAHAN_BSLR,
    LM_WELFORD_BSLR,
    LM_FAST_LR,
    LM_WELFORD_LR,
};

struct TRunOptions {
    TString FeaturesPath;
    TString ModelPath;

    ELearningMode LearningMode = ELearningMode::LM_WELFORD_LR;

    void AddOpts(NLastGetopt::TOpts& opts) {
        opts.AddLongOption('f', "features", "learn features file path")
            .Required()
            .StoreResult(&FeaturesPath);
        opts.AddLongOption('m', "model", "model path")
            .Optional()
            .StoreResult(&ModelPath);

        opts.AddLongOption("fast-bslr", "fast bslr learning mode")
            .Optional()
            .NoArgument()
            .StoreValue(&LearningMode, ELearningMode::LM_FAST_BSLR);
        opts.AddLongOption("kahan-bslr", "kahan bslr learning mode")
            .Optional()
            .NoArgument()
            .StoreValue(&LearningMode, ELearningMode::LM_KAHAN_BSLR);
        opts.AddLongOption("welford-bslr", "welford bslr learning mode")
            .Optional()
            .NoArgument()
            .StoreValue(&LearningMode, ELearningMode::LM_WELFORD_BSLR);

        opts.AddLongOption("fast-lr", "fast linear regression learning mode")
            .Optional()
            .NoArgument()
            .StoreValue(&LearningMode, ELearningMode::LM_FAST_LR);
        opts.AddLongOption("welford-lr", "welford linear regression learning mode (is default)")
            .Optional()
            .NoArgument()
            .StoreValue(&LearningMode, ELearningMode::LM_WELFORD_LR);
    }
};

struct TRegressionMetrics {
    double RMSE;
    double TargetStdDev;
    double DeterminationCoefficient;
};

template <typename TLearner>
TLinearModel LearnLinearModel(const NEthos::TDensePool& pool) {
    TLearner learner;
    for (const NEthos::TDenseInstance& instance : pool) {
        learner.Add(instance.Features, instance.Target, instance.Weight);
    }
    return learner.Solve();
}

TRegressionMetrics Metrics(const NEthos::TDensePool& pool, const TLinearModel& linearModel) {
    TKahanAccumulator<double> sumSquaredErrors;

    TMeanCalculator mseCalculator;
    TDeviationCalculator deviationCalculator;

    for (const NEthos::TDenseInstance& instance : pool) {
        const double prediction = linearModel.Prediction(instance.Features);
        const double error = prediction - instance.Target;

        sumSquaredErrors += error * error * instance.Weight;

        mseCalculator.Add(error * error, instance.Weight);
        deviationCalculator.Add(instance.Target, instance.Weight);
    }

    TRegressionMetrics regressionMetrics;
    regressionMetrics.RMSE = sqrt(Max(0., mseCalculator.GetMean()));
    regressionMetrics.TargetStdDev = deviationCalculator.GetStdDev();
    regressionMetrics.DeterminationCoefficient = 1. - sumSquaredErrors.Get() / deviationCalculator.GetDeviation();

    return regressionMetrics;
}

int DoLearn(int argc, const char** argv) {
    TRunOptions runOptions;
    {
        NLastGetopt::TOpts opts;
        opts.SetFreeArgsMax(0);

        runOptions.AddOpts(opts);

        NLastGetopt::TOptsParseResult res(&opts, argc, argv);
    }

    NEthos::TDensePool pool = NEthos::TDensePool::FromFeatures(runOptions.FeaturesPath);

    TLinearModel linearModel;
    switch (runOptions.LearningMode) {
    case ELearningMode::LM_FAST_BSLR:    linearModel = LearnLinearModel<TFastBestSLRSolver>(pool); break;
    case ELearningMode::LM_KAHAN_BSLR:   linearModel = LearnLinearModel<TKahanBestSLRSolver>(pool); break;
    case ELearningMode::LM_WELFORD_BSLR: linearModel = LearnLinearModel<TBestSLRSolver>(pool); break;
    case ELearningMode::LM_FAST_LR:      linearModel = LearnLinearModel<TFastLinearRegressionSolver>(pool); break;
    case ELearningMode::LM_WELFORD_LR:   linearModel = LearnLinearModel<TLinearRegressionSolver>(pool); break;
    }

    if (runOptions.ModelPath) {
        TFixedBufferFileOutput modelOut(runOptions.ModelPath);
        linearModel.Save(&modelOut);
    }

    TRegressionMetrics metrics = Metrics(pool, linearModel);

    Cout << "learn rmse:           " << Sprintf("%.6lf", metrics.RMSE) << Endl;
    Cout << "learn R^2:            " << Sprintf("%.6lf", metrics.DeterminationCoefficient) << Endl;
    Cout << "learn target stddev:  " << Sprintf("%.6lf", metrics.TargetStdDev) << Endl;

    return 0;
}

int DoTest(int argc, const char** argv) {
    TRunOptions runOptions;
    {
        NLastGetopt::TOpts opts;
        opts.SetFreeArgsMax(0);

        runOptions.AddOpts(opts);

        NLastGetopt::TOptsParseResult res(&opts, argc, argv);
    }

    NEthos::TDensePool pool = NEthos::TDensePool::FromFeatures(runOptions.FeaturesPath);

    TLinearModel linearModel;
    if (runOptions.ModelPath) {
        TFileInput modelIn(runOptions.ModelPath);
        linearModel.Load(&modelIn);
    }

    TRegressionMetrics metrics = Metrics(pool, linearModel);

    Cout << "test rmse:            " << Sprintf("%.6lf", metrics.RMSE) << Endl;
    Cout << "test R^2:             " << Sprintf("%.6lf", metrics.DeterminationCoefficient) << Endl;
    Cout << "test target stddev:   " << Sprintf("%.6lf", metrics.TargetStdDev) << Endl;

    return 0;
}

int DoApply(int argc, const char** argv) {
    TRunOptions runOptions;
    {
        NLastGetopt::TOpts opts;
        opts.SetFreeArgsMax(0);

        runOptions.AddOpts(opts);

        NLastGetopt::TOptsParseResult res(&opts, argc, argv);
    }

    TLinearModel linearModel;
    if (runOptions.ModelPath) {
        TFileInput modelIn(runOptions.ModelPath);
        linearModel.Load(&modelIn);
    }

    {
        auto input = OpenInput(runOptions.FeaturesPath);
        TString featuresStr;
        while (input->ReadLine(featuresStr)) {
            TStringBuf featuresStrBuf(featuresStr);
            Cout << featuresStrBuf.NextTok('\t') << '\t';
            Cout << featuresStrBuf.NextTok('\t') << '\t';
            Cout << featuresStrBuf.NextTok('\t') << '\t';
            Cout << featuresStrBuf.NextTok('\t') << '\t';

            NEthos::TDenseInstance instance = NEthos::TDenseInstance::FromFeaturesString(featuresStr);
            Cout << linearModel.Prediction(instance.Features) << "\n";
        }
    }

    return 0;
}

int DoPrint(int argc, const char** argv) {
    TString modelPath;
    {
        NLastGetopt::TOpts opts;
        opts.SetFreeArgsMax(0);

        opts.AddLongOption("model", "model path")
            .Required()
            .StoreResult(&modelPath);

        NLastGetopt::TOptsParseResult res(&opts, argc, argv);
    }

    TLinearModel linearModel;
    if (modelPath) {
        TFileInput modelIn(modelPath);
        linearModel.Load(&modelIn);
    }

    Cout << JoinSeq("\t", linearModel.GetCoefficients()) << "\t" << linearModel.GetIntercept() << Endl;

    return 0;
}

int DoUnimodal(int argc, const char** argv) {
    Y_UNUSED(argc && argv);

    TString dataStr;
    while (Cin.ReadLine(dataStr)) {
        TVector<double> values;

        TStringBuf dataStrBuf(dataStr);
        while (dataStrBuf) {
            values.push_back(FromString<double>(dataStrBuf.NextTok('\t')));
        }

        const double determination = MakeUnimodal(values);

        for (const double value : values) {
            Cout << Sprintf("%.3lf", value) << "\t";
        }
        Cout << "R^2: " << determination << Endl;
    }

    return 0;
}

int main(int argc, const char** argv) {
    TModChooser modChooser;
    modChooser.AddMode("learn", DoLearn, "learn linear regression model");
    modChooser.AddMode("test", DoTest, "test linear regression model");
    modChooser.AddMode("apply", DoApply, "apply linear regression model");
    modChooser.AddMode("print", DoPrint, "print linear model");
    modChooser.AddMode("unimodal", DoUnimodal, "do unimodal transformation on numbers");
    return modChooser.Run(argc, argv);
}
