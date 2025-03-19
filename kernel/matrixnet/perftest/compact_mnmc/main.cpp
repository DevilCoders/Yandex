#include <kernel/matrixnet/mn_multi_categ.h>
#include <util/stream/file.h>
#include <util/datetime/cputimer.h>
#include <library/cpp/getopt/small/last_getopt_parse_result.h>
#include <library/cpp/getopt/last_getopt.h>
#include <library/cpp/getopt/modchooser.h>
#include <quality/relev_tools/mx_ops/lib/util.h>

const int N_EXAMPLES = 3;
const int N_FEATURES = 30;

float FEATURES[N_EXAMPLES][N_FEATURES] = {
        {-2.61505093107, -2.64535816675, -2.75971760783, -2.95361547792, -3.06964980986, -3.20704761816, -3.44855706466,
         -3.55418390431, -3.67202341163, -3.73364897877, -0.226758165089, -0.905593710769, -1.41216566322,
         -1.80543999988, -1.9911605389, -2.2873672818, -2.52728578878, -2.67378225642, -2.8076984389, -2.94586742531,
         -1.80627417549, -1.32373047767, -1.01066471611, -0.823115435781, -0.740511972419, -0.631303664238,
         -0.572013601398, -0.534657881408, -0.520662024815, -0.530685337451},
        {-3.22587491585, -3.38070334977, -3.53075448687, -3.75722321643, -3.91822301544, -4.09057384492,
         -4.33295343632, -4.46894675119, -4.65654575462, -4.69760641172, -0.0510994502126, -0.839663423809,
         -1.43255957001, -1.8332477805, -2.05635669236, -2.35650115008, -2.64861915152, -2.83256547562, -2.95538264279,
         -3.11390258796, -2.63073099909, -2.05707438444, -1.65213162395, -1.4084368348, -1.31556341568, -1.2100784967,
         -1.14398641752, -1.12420319344, -1.11219319597, -1.12774621298},
        {-0.0601516399778, -0.183121064633, -0.362865076252, -0.670548808479, -0.891404747005, -1.09862335337,
                -1.45293212703, -1.67579580646, -1.89436208368, -1.94078040346, -0.528456523896, -2.75958446389,
                -3.76679610724, -4.20118645767, -4.43063003996, -4.72918320122, -4.95272777101, -5.20822908311,
                -5.35681646072, -5.48789794919,  0.138191106731, 1.7781150057,    2.36505732008,   2.59170460768,
                2.64946462772,   2.7719528884,    2.79460964153,   2.79393041456,
                2.81912465758,   2.87274565166}
};


void Print(const TVector<double>& vec, size_t ndocs, size_t nclasses) {
    Y_ASSERT(ndocs * nclasses <= vec.size());
    for (size_t d = 0; d < ndocs; ++d) {
        Cout << "doc" << d << ":";
        for (size_t i = 0; i < nclasses; ++i) {
            Cout << " " << vec[i + d * nclasses];
        }
        Cout << Endl;
    }
}

template <typename Op>
inline ui64 Profile(size_t count, Op op) {
    TProfileTimer t;
    ui64 totalDuration = 0;
    for (size_t i = 0; i < count; ++i) {
        op();
        totalDuration += t.Step().GetValue();
    }
    return totalDuration / count;
}

void LoadFeatures(const TString& path, const size_t linesLimit, TVector<TVector<float>>& docs)
{
    if (path) {
        TFileInput in(path);
        TString line;
        size_t lineCounter = 0;
        for (; lineCounter < linesLimit && in.ReadLine(line); ++lineCounter) {
            TVector<TString> ch;
            Split(line, "\t", ch);
            docs.push_back(TVector<float>());
            ReadFactors(ch.begin() + 5, ch.end(), docs.back());
        }
        Cout << "Read " << lineCounter << " lines from pool file." << Endl;

    } else {
        const size_t NDOCS = 10;

        for (size_t i = 0; i < NDOCS; ++i) {
            size_t example = i % N_EXAMPLES;
            TVector<float> features(FEATURES[example], FEATURES[example] + N_FEATURES);
            docs.push_back(features);
        }
    }
}

int main(int argc, char** argv)
{
    TString poolPath;
    TString mnmcPath;
    TString mnmcCompactPath;
    size_t profileTimes = 100;
    size_t poolLinesLimit = Max();

    {
        NLastGetopt::TOpts opts;
        opts.SetFreeArgsMax(0);

        opts.AddLongOption("pool", "features pool path")
                .Optional()
                .StoreResult(&poolPath);
        opts.AddLongOption("pool-limit", "number of lines to read from pool")
                .Optional()
                .StoreResult(&poolLinesLimit);
        opts.AddLongOption("mnmc", "mnmc model")
                .Optional()
                .StoreResult(&mnmcPath);
        opts.AddLongOption("mnmc-compact", "compact mnmc model")
                .Optional()
                .StoreResult(&mnmcCompactPath);
        opts.AddLongOption("profile-times", "number of times to run for profiling")
                .Optional()
                .StoreResult(&profileTimes);

        NLastGetopt::TOptsParseResult res(&opts, argc, argv);
    }

    TVector<TVector<float>> docs;
    LoadFeatures(poolPath, poolLinesLimit, docs);
    const size_t ndocs = docs.size();

    if (mnmcPath) {
        NMatrixnet::TMnMultiCateg mnmc;
        {
            TFileInput in(mnmcPath);
            mnmc.Load(&in);
            Cout << "Loaded regular!" << Endl;
        }

        const size_t nclasses = mnmc.CategValues().size();

        TVector<double> result(ndocs * nclasses);
        mnmc.CalcCategoriesRanking(docs, ~result);
        Cout << "Regular MNMC result: " << Endl;
        Print(result, Min(ndocs, 8ul), nclasses);

        if (profileTimes > 0) {
            Cout << "Average regular time = " << Profile(profileTimes, [&]() { mnmc.CalcCategoriesRanking(docs, ~result); }) << Endl;
        }
    }

    if (mnmcCompactPath) {
        NMatrixnet::TMnMultiCateg mnmcCompact;
        {
            TFileInput in(mnmcCompactPath);
            mnmcCompact.Load(&in);
            Cout << "Loaded compact!" << Endl;
        }

        const size_t nclasses = mnmcCompact.CategValues().size();

        TVector<double> resultCompact(ndocs * nclasses);
        mnmcCompact.CalcCategoriesRanking(docs, ~resultCompact);
        Cout << "Compact MNMC result: " << Endl;
        Print(resultCompact, Min(ndocs, 8ul), nclasses);

        if (profileTimes > 0) {
            Cout << "Average compact time = " << Profile(profileTimes, [&]() { mnmcCompact.CalcCategoriesRanking(docs, ~resultCompact); }) << Endl;
        }
    }
}
