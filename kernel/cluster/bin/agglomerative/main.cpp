#include <kernel/cluster/lib/agglomerative/agglomerative_clustering.h>
#include <kernel/cluster/lib/cluster_metrics/base.h>

#include <library/cpp/getopt/last_getopt.h>

#include <util/stream/output.h>
#include <util/stream/file.h>
#include <util/string/cast.h>
#include <util/string/split.h>
#include <util/system/fs.h>

class TClusterizationInfo {
private:
    struct TElementsPair {
        ui32 LeftNumber;
        ui32 RightNumber;
        float Similarity;

        TElementsPair(ui32 leftNumber = 0,
                      ui32 rightNumber = 0,
                      float similarity = 0)
            : LeftNumber(leftNumber)
            , RightNumber(rightNumber)
            , Similarity(similarity)
        {}

        TElementsPair(const TElementsPair& source)
            : LeftNumber(source.LeftNumber)
            , RightNumber(source.RightNumber)
            , Similarity(source.Similarity)
        {}

        template <typename S>
        void SaveLoad(S* s) {
            ::SaveLoad(s, LeftNumber);
            ::SaveLoad(s, RightNumber);
            ::SaveLoad(s, Similarity);
        }
    };

    TVector<TElementsPair> Pairs;

    TVector<TString> Urls;
    THashMap<TString, ui32> UrlNumbers;
public:
    template <typename S>
    void SaveLoad(S* s) {
        ::SaveLoad(s, Pairs);
        ::SaveLoad(s, Urls);
        ::SaveLoad(s, UrlNumbers);
    }

    bool Load(const TString& indexFileName, bool readFromPredictions) {
        if (!indexFileName) {
            Read(readFromPredictions);
            return true;
        }

        try {
            TFileInput in(indexFileName);
            ::Load(&in, *this);
        } catch (...) {
            Cerr << "cannot load from " << indexFileName << ": "
                 << CurrentExceptionMessage() << "\n"
                 << "loading from cin...\n";
        }

        if (!ElementsCount()) {
            Read(readFromPredictions);
            return false;
        }

        return true;
    }

    void TrySerialize(const TString& indexFileName) {
        if (!indexFileName) {
            return;
        }
        try {
            TFixedBufferFileOutput out(indexFileName);
            ::Save(&out, *this);
        } catch (...) {
            Cerr << "cannot save index: " << indexFileName << "\n";
        }
    }

    void Read(bool readPredictions) {
        if (readPredictions) {
            ReadFromPredictions(Cin);
        } else {
            ReadSimple(Cin);
        }
    }

    void ReadFromPredictions(IInputStream& in) {
        TString dataStr;
        while (in.ReadLine(dataStr)) {
            TStringBuf dataStrBuf(dataStr);

            dataStrBuf.NextTok('\t');
            dataStrBuf.NextTok('\t');

            ui32 leftAlias = GetUrlNumber(ToString(dataStrBuf.NextTok(' ')));
            ui32 rightAlias = GetUrlNumber(ToString(dataStrBuf.NextTok('\t')));

            dataStrBuf.NextTok('\t');
            float similarity = FromString<float>(dataStrBuf.NextTok('\t'));

            Pairs.push_back(TElementsPair(leftAlias, rightAlias, similarity));
        }
        Pairs.shrink_to_fit();
    }

    void ReadSimple(IInputStream& in) {
        TString dataStr;
        while (in.ReadLine(dataStr)) {
            TStringBuf dataStrBuf(dataStr);

            ui32 leftAlias = GetUrlNumber(ToString(dataStrBuf.NextTok('\t')));
            ui32 rightAlias = GetUrlNumber(ToString(dataStrBuf.NextTok('\t')));
            float similarity = FromString<float>(dataStrBuf.NextTok('\t'));

            Pairs.push_back(TElementsPair(leftAlias, rightAlias, similarity));
        }
        Pairs.shrink_to_fit();
    }

    size_t ElementsCount() const {
        return Urls.size();
    }

    void FillClusterization(NAgglomerative::TClusterization& clusterization) const {
        for (size_t pairNumber = 0; pairNumber < Pairs.size(); ++pairNumber) {
            const TElementsPair& pair = Pairs[pairNumber];
            clusterization.Add(pair.LeftNumber, pair.RightNumber, pair.Similarity);
        }
    }

    NClusterMetrics::TClusterization GetMeasurableClusterization(
        const NAgglomerative::TClusterization& clusterization,
        bool includeOneDocClusters = false
    ) const {
        NClusterMetrics::TClusterization result;
        for (ui32 clusterNumber = 0; clusterNumber < clusterization.GetClustersCount(); ++clusterNumber) {
            const TVector<std::pair<ui32, float>>& cluster = clusterization.GetCluster(clusterNumber);
            if (cluster.size() < 2 && !includeOneDocClusters) {
                continue;
            }
            for (const auto& [docId, sim]: cluster) {
                result.Add(Urls[docId], ToString(clusterNumber), sim);
            }
        }
        return result;
    }
private:
    ui32 GetUrlNumber(const TString& url) {
        ui32& urlNumber = UrlNumbers[url];
        if (Urls.size() != UrlNumbers.size()) {
            urlNumber = Urls.size();
            Urls.push_back(url);
        }
        return urlNumber;
    }
};

int main(int argc, char** argv) {
    TString markupFileName;
    TString indexFile;

    float recallFactor = 1;
    float recallDecayFactor = 0.f;

    float similarityThreshold = NAgglomerative::DefaultSimilarityThreshold;

    float positiveProbabilityFactor = 0;
    bool doRegularization = true;

    size_t threadsCount = 5;

    bool readFromPredictions = false;
    bool setupRelevances = false;
    bool includeOneDocClusters = false;

    try {
        NLastGetopt::TOpts opts;

        opts.AddCharOption('m', "markup filename").StoreResult(&markupFileName);

        opts.AddCharOption('f', "recall against precision factor")
            .StoreResult(&recallFactor).DefaultValue(ToString(recallFactor));
        opts.AddCharOption('d', "recall against precision decay factor")
            .StoreResult(&recallDecayFactor).DefaultValue(ToString(recallDecayFactor));
        opts.AddCharOption('s', "similarity threshold")
            .StoreResult(&similarityThreshold).DefaultValue(ToString(similarityThreshold));

        opts.AddCharOption('p', "probability of unmarked document to be relevant")
            .StoreResult(&positiveProbabilityFactor).DefaultValue(ToString(positiveProbabilityFactor));
        opts.AddCharOption('r', "do regularization (1) or not (0)")
            .StoreResult(&doRegularization).DefaultValue(ToString(doRegularization));

        opts.AddCharOption('T', "threads count")
            .StoreResult(&threadsCount).DefaultValue(ToString(threadsCount));

        opts.AddLongOption("index", "pairs similarity index").StoreResult(&indexFile);

        opts.AddLongOption("predictions", "read in predictions format").NoArgument().StoreValue(&readFromPredictions, true);
        opts.AddLongOption("relevances", "setup relevances").NoArgument().StoreValue(&setupRelevances, true);
        opts.AddLongOption("onedocs", "include one doc clusters").NoArgument().StoreValue(&includeOneDocClusters, true);

        NLastGetopt::TOptsParseResult res(&opts, argc, argv);
    } catch (yexception& ex) {
        Cerr << "Error parsing command line options: " << ex.what() << "\n";
        return 1;
    }

    TVector<NAgglomerative::TRawSimilaritiesHash> rawSimilarities;

    TClusterizationInfo clusterizationInfo;
    clusterizationInfo.Load(indexFile, readFromPredictions);
    clusterizationInfo.TrySerialize(indexFile);

    NAgglomerative::TClusterization clusterization(
        clusterizationInfo.ElementsCount(),
        recallFactor,
        recallDecayFactor,
        similarityThreshold,
        setupRelevances ? &rawSimilarities : nullptr,
        threadsCount
    );

    clusterizationInfo.FillClusterization(clusterization);

    clusterization.Build();
    clusterization.SetupRelevances();

    NClusterMetrics::TClusterization measurableClusterization = clusterizationInfo.GetMeasurableClusterization(
        clusterization,
        includeOneDocClusters
    );

    Cerr << "agglomerative precision: " << clusterization.GetPrecision() << "\n";
    Cerr << "agglomerative recall:    " << clusterization.GetRecall() << "\n";
    Cerr << "iterations done:         " << clusterization.GetIterationsCount() << "\n";
    Cerr << "\n";

    if (!!markupFileName) {
        NClusterMetrics::TClusterization markup;
        markup.Read(markupFileName);
        markup.SetupMetrics(measurableClusterization);
        NClusterMetrics::PrintIntegralMetrics(Cout, markup.GetIntegralMetrics(), markup.GetIntegralMetricsBounds());
    } else {
        measurableClusterization.Print(Cout);
    }

    return 0;
}
