#include <ml/differential_evolution/differential_evolution.h>

#include <library/cpp/getopt/last_getopt.h>
#include <library/cpp/vowpalwabbit/vowpal_wabbit_model.h>

#include <util/stream/direct_io.h>
#include <util/generic/vector.h>
#include <util/generic/string.h>
#include <util/generic/ymath.h>

int PackVowpalWabbitModel(int argc, const char** argv){
    auto opts = NLastGetopt::TOpts::Default();
    opts.SetFreeArgsNum(0);

    TString modelFileName;
    TString packedModelFileName;
    float minMultiplier = -300;
    float maxMultiplier = 300;
    ui32 threadCount = 0;
    constexpr ui32 populationSize = 48;

    opts.AddLongOption("model", "[in] Binary VW model").Required().RequiredArgument("model").StoreResult(&modelFileName);
    opts.AddLongOption("packed", "[out] Packed VW model where all weights converted from float to i8").Required().RequiredArgument("packed").StoreResult(&packedModelFileName);
    opts.AddLongOption("threads", "Thread to use for model optimization").Required().RequiredArgument("threads").StoreResult(&threadCount);
    opts.AddLongOption("min_multiplier", "Min multiplier for i8 to float conversion").Optional().RequiredArgument("min_multiplier").StoreResult(&minMultiplier).DefaultValue(-300);
    opts.AddLongOption("max_multiplier", "Max multiplier for i8 to float conversion").Optional().RequiredArgument("max_multiplier").StoreResult(&maxMultiplier).DefaultValue(300);

    NLastGetopt::TOptsParseResult parsedOpts(&opts, argc, argv);
    Y_VERIFY(minMultiplier < maxMultiplier);
    Y_VERIFY(threadCount > 0 && threadCount < 1000);

    TVowpalWabbitModel model(modelFileName);
    TVector<float> weights(model.GetWeights(), model.GetWeights() + model.GetWeightsSize());
    std::sort(weights.begin(), weights.end(), [] (float w1, float w2) {
        return std::abs(w1) < std::abs(w2);
    });

    auto fun = [&weights] (const TVector<double>& params) {
        const double multiplier = params[0];
        double loss = 0;

        for (float w : weights) {
            long packedVal = std::clamp(lround(w / multiplier), static_cast<long>(std::numeric_limits<i8>::min()),
                                        static_cast<long>(std::numeric_limits<i8>::max()));
            double restored = packedVal * multiplier;
            loss += Sqr(w - restored);
        }

        return loss;
    };

    NDiffEvolution::TMultiThreadedEvaluator evaluator(fun, threadCount);
    TVector<NDiffEvolution::TBound> bound(1, NDiffEvolution::TBound(minMultiplier, maxMultiplier));
    NDiffEvolution::TPopulation population(populationSize, bound);
    NDiffEvolution::TEvolutionParams evolutionParams;

    population.Evolve(evolutionParams, &evaluator);

    double loss = population.GetBest().Score;
    ui32 notImprovedCount = 0;
    ui32 iteration = 0;

    printf("Searching for optimal multiplier for i8 to float conversion in (%f, %f) range using %u threads...\n", minMultiplier, maxMultiplier, threadCount);
    while (notImprovedCount < 20) {
        ++iteration;
        population.Evolve(evolutionParams, &evaluator);
        double newLoss = population.GetBest().Score;
        if (newLoss < loss) {
            loss = newLoss;
            notImprovedCount = 0;
            printf("RMSE loss = %f at iteration %u, multiplier = %f\n", std::sqrt(loss / weights.size()), iteration, population.GetBest().Params[0]);
        } else {
            ++notImprovedCount;
        }
    }

    const float multiplier = population.GetBest().Params[0];
    {
        TBufferedFileOutputEx output(packedModelFileName, CreateAlways);
        output.Write(&multiplier, sizeof(multiplier));
        for (size_t i = 0; i < weights.size(); ++i) {
            const i8 w = std::clamp(lround(model.GetWeights()[i] / multiplier),
                                    static_cast<long>(std::numeric_limits<i8>::min()),
                                    static_cast<long>(std::numeric_limits<i8>::max()));
            output.Write(w);
        }
    }
    NVowpalWabbit::TPackedModel packedModel(packedModelFileName);
    Y_VERIFY(packedModel.GetMultiplier() == multiplier);
    Y_VERIFY(packedModel.GetBits() == model.GetBits());
    printf("Packed model have been saved to %s\n", packedModelFileName.c_str());
    return 0;
}
